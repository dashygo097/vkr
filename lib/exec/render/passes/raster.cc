#include "vkr/exec/render/passes/raster.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string_view>

namespace vkr::exec {
namespace {

void bindSource(pipe::DescriptorSet &set, std::string_view passName,
                size_t sourceIndex, const pipe::OffscreenTarget &source,
                const RenderPassInputDesc &input) {
  switch (input.kind) {
  case RenderPassInputKind::Color: {
    if (!source.hasColor() || !source.color().hasSampler()) {
      VKR_EXEC_ERROR(
          "RasterPass '{}' source {} needs a sampled color attachment",
          std::string(passName), sourceIndex);
    }
    set.texture(input.binding, source.color());
    break;
  }
  case RenderPassInputKind::Depth: {
    const auto *depth = source.depth();
    if (depth == nullptr || !depth->hasSampler()) {
      VKR_EXEC_ERROR(
          "RasterPass '{}' source {} needs a sampled depth attachment",
          std::string(passName), sourceIndex);
    }
    set.texture(input.binding, *depth);
    break;
  }
  }
}

} // namespace

RasterPass::RasterPass(RenderExecutor &executor, const core::Device &device)
    : executor_(executor), device_(device), descriptor_pool_(device) {
  descriptor_sets_.resize(executor_.framesInFlight());
}

RasterPass::~RasterPass() { destroy(); }

void RasterPass::create() {
  validate(desc_);
  for (const auto &mesh : meshes_) {
    validateMesh(mesh.get(), desc_.pipeline.vertexInput);
  }
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void RasterPass::destroy() noexcept {
  pipeline_.reset();
  for (auto &frame : descriptor_sets_) {
    for (auto &set : frame) {
      set.destroy();
    }
  }
  for (auto &layout : descriptor_layouts_) {
    layout->destroy();
  }
  descriptor_pool_.destroy();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void RasterPass::update(const RasterPassDesc &desc) {
  ensureConfigurable();
  if (target_ || render_pass_ || pipeline_ || descriptor_pool_.valid()) {
    VKR_EXEC_ERROR("RasterPass '{}' must be destroyed before updating its "
                   "configuration",
                   name());
  }
  validate(desc);
  for (const auto &mesh : meshes_) {
    validateMesh(mesh.get(), desc.pipeline.vertexInput);
  }
  auto nextDesc = desc;
  for (const auto &input : nextDesc.inputs) {
    if (input.setIndex >= nextDesc.descriptorLayouts.size()) {
      nextDesc.descriptorLayouts.resize(static_cast<size_t>(input.setIndex) +
                                        1);
    }
  }
  const auto setCount = std::max<size_t>(1, nextDesc.descriptorLayouts.size());
  while (descriptor_layouts_.size() > setCount) {
    for (auto &frame : descriptor_sets_) {
      frame.pop_back();
    }
    descriptor_layouts_.pop_back();
  }
  while (descriptor_layouts_.size() < setCount) {
    descriptor_layouts_.push_back(
        std::make_unique<pipe::DescriptorSetLayout>(device_));
    for (auto &frame : descriptor_sets_) {
      frame.emplace_back(device_, descriptor_pool_,
                         *descriptor_layouts_.back());
    }
  }
  desc_ = std::move(nextDesc);
}

auto RasterPass::mesh(const scene::IMesh &mesh) -> RasterPass & {
  const auto &input =
      pipeline_ ? pipeline_->desc().vertexInput : desc_.pipeline.vertexInput;
  validateMesh(mesh, input);
  const auto existing = std::find_if(
      meshes_.begin(), meshes_.end(), [&mesh](const auto &entry) -> bool {
        return std::addressof(entry.get()) == std::addressof(mesh);
      });
  if (existing == meshes_.end()) {
    meshes_.emplace_back(mesh);
  }
  return *this;
}

void RasterPass::validateMesh(const scene::IMesh &mesh,
                              const scene::VertexInputDesc &expected) const {
  const auto vertices = mesh.vertexBufferBase();
  const auto indices = mesh.indexBuffer();
  if (!vertices || !indices || vertices->get().buffer() == VK_NULL_HANDLE ||
      indices->get().buffer() == VK_NULL_HANDLE ||
      vertices->get().vertexCount() == 0 || indices->get().indices().empty()) {
    VKR_EXEC_ERROR("RasterPass '{}' requires initialized mesh buffers", name());
  }
  const auto input = vertices->get().vertexInputDesc();
  if (!(input == expected) || input.bindings.size() != 1 ||
      input.bindings[0].binding != 0 ||
      input.bindings[0].inputRate != VK_VERTEX_INPUT_RATE_VERTEX) {
    VKR_EXEC_ERROR("RasterPass '{}' mesh has incompatible vertex input",
                   name());
  }
}

auto RasterPass::addSource(Pass &source) -> RasterPass & {
  ensureConfigurable();
  sources_.emplace_back(source);
  return *this;
}

auto RasterPass::setSources(std::vector<std::reference_wrapper<Pass>> sources)
    -> RasterPass & {
  ensureConfigurable();
  sources_ = std::move(sources);
  return *this;
}

void RasterPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_EXEC_ERROR("RasterPass '{}' recorded before create", name());
  }

  executor_.beginProfileScope(name());
  executor_.beginPass(*render_pass_, *framebuffers_, desc_.clearValues);
  executor_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    if (descriptor_pool_.valid()) {
      executor_.bindPipeline(*pipeline_,
                             descriptor_sets_.at(executor_.frameIndex()));
    } else {
      executor_.bindPipeline(*pipeline_);
    }
    for (const auto &entry : meshes_) {
      const auto &mesh = entry.get();
      const auto vertices = mesh.vertexBufferBase();
      const auto indices = mesh.indexBuffer();
      if (!vertices || !indices) {
        VKR_EXEC_ERROR("RasterPass '{}' has a mesh with invalid buffers",
                       name());
      }
      executor_.drawIndexed(vertices->get(), indices->get());
    }
  }

  executor_.endPass();
  executor_.endProfileScope();
}

auto RasterPass::target() -> pipe::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

auto RasterPass::target() const -> const pipe::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

void RasterPass::createTarget() {
  target_ = std::make_unique<pipe::OffscreenTarget>(device_);
  target_->update(desc_.target);
}

void RasterPass::createRenderPass() {
  render_pass_ = std::make_unique<pipe::RenderPass>(device_);
  pipe::RenderPassDesc renderPassDesc{};

  if (target_->hasColor()) {
    renderPassDesc = pipe::RenderPassDesc::makeOffscreen(
        target_->color().desc().format, target_->depth()
                                            ? target_->depth()->desc().format
                                            : VK_FORMAT_UNDEFINED);

    const auto &colorDesc = target_->color().desc();
    const auto colorFinalLayout = colorDesc.finalLayout;
    if (colorFinalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      renderPassDesc.colors[0].finalLayout = colorFinalLayout;
    } else if ((colorDesc.usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) == 0) {
      renderPassDesc.colors[0].finalLayout =
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  } else if (target_->depth()) {
    renderPassDesc = pipe::RenderPassDesc::makeDepthOnly(
        target_->depth()->desc().format);
  } else {
    VKR_EXEC_ERROR("RasterPass '{}' target has no attachments", name());
  }

  if (target_->depth()) {
    const auto &depthDesc = target_->depth()->desc();
    if (depthDesc.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      renderPassDesc.depth.finalLayout = depthDesc.finalLayout;
    }

    renderPassDesc.depth.storeOp =
        (depthDesc.usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0
            ? VK_ATTACHMENT_STORE_OP_STORE
            : depthDesc.storeOp;
  }

  render_pass_->update(renderPassDesc);
}

void RasterPass::createFramebuffers() {
  auto framebufferDesc = FramebuffersDesc::single(
      target_->width(), target_->height(), target_->attachmentViews());

  framebuffers_ = std::make_unique<Framebuffers>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void RasterPass::validate(const RasterPassDesc &desc) const {
  if (!desc.target.isValid()) {
    VKR_EXEC_ERROR("RasterPass '{}' has an invalid target descriptor", name());
  }
  if (!desc.pipeline.isValid()) {
    VKR_EXEC_ERROR("RasterPass '{}' has an invalid pipeline descriptor",
                   name());
  }
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.physicalDevice(), &properties);
  auto layouts = desc.descriptorLayouts;
  if (layouts.size() > properties.limits.maxBoundDescriptorSets) {
    VKR_EXEC_ERROR("RasterPass '{}' exceeds maxBoundDescriptorSets", name());
  }
  for (const auto &input : desc.inputs) {
    if (input.setIndex >= properties.limits.maxBoundDescriptorSets) {
      VKR_EXEC_ERROR("RasterPass '{}' input set index is out of range", name());
    }
    if (input.setIndex >= layouts.size()) {
      layouts.resize(static_cast<size_t>(input.setIndex) + 1);
    }
    layouts[input.setIndex].texture(input.binding, input.stageFlags);
  }
  for (const auto &layoutDesc : layouts) {
    if (!layoutDesc.isValid()) {
      VKR_EXEC_ERROR("RasterPass '{}' requires unique bindings within each set "
                     "with nonzero counts/stages",
                     name());
    }
    for (const auto &binding : layoutDesc.bindings) {
      const auto &layout = binding.layout;
      if (layout.descriptorCount != 1 ||
          (layout.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
           layout.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
           layout.descriptorType !=
               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
        VKR_EXEC_ERROR(
            "RasterPass '{}' has an unsupported descriptor binding {}", name(),
            layout.binding);
      }
    }
  }
}

void RasterPass::createDescriptors() {
  const auto inputs = desc_.inputs;
  if (inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR("RasterPass '{}' input count mismatch", name());
  }
  auto layouts = desc_.descriptorLayouts;
  for (const auto &input : inputs) {
    if (input.setIndex >= layouts.size()) {
      layouts.resize(static_cast<size_t>(input.setIndex) + 1);
    }
    layouts[input.setIndex].texture(input.binding, input.stageFlags);
  }
  if (layouts.empty()) {
    return;
  }
  descriptor_pool_.update(
      pipe::DescriptorPoolDesc::sets(layouts, executor_.framesInFlight()));
  for (uint32_t setIndex = 0; setIndex < layouts.size(); ++setIndex) {
    descriptor_layouts_[setIndex]->update(layouts[setIndex]);
    for (auto &frame : descriptor_sets_) {
      frame[setIndex].create();
    }
  }
  for (size_t index = 0; index < sources_.size(); ++index) {
    const auto source =
        sources_[index].get().capability<RenderTargetCapability>();
    if (!source) {
      VKR_EXEC_ERROR("RasterPass '{}' source '{}' has no render target", name(),
                     sources_[index].get().name());
    }
    for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
      bindSource(descriptor_sets_[frame].at(inputs[index].setIndex), name(),
                 index, source->get().target(frame), inputs[index]);
    }
  }
}

void RasterPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  if (pipelineDesc.layout.setLayouts.empty()) {
    for (const auto &layout : descriptor_layouts_) {
      if (layout->valid()) {
        pipelineDesc.layout.setLayouts.push_back(layout->layout());
      }
    }
  }

  pipeline_ =
      std::make_unique<pipe::GraphicsPipeline>(device_, *render_pass_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("RasterPass '{}' failed to create graphics pipeline '{}'",
                   name(), pipelineDesc.name);
  }
}

} // namespace vkr::exec
