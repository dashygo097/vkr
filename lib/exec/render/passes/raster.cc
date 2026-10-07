#include "vkr/exec/render/passes/raster.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string_view>

namespace vkr::exec {
namespace {

auto imageLayoutForAttachment(const ColorAttachment &attachment)
    -> VkImageLayout {
  return attachment.desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
             ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             : attachment.desc().finalLayout;
}

auto sourceImageInfo(std::string_view passName, size_t sourceIndex,
                     const OffscreenTarget &source,
                     const RenderPassInputDesc &input)
    -> VkDescriptorImageInfo {
  VkDescriptorImageInfo imageInfo{};

  switch (input.kind) {
  case RenderPassInputKind::Color: {
    if (!source.hasColor()) {
      VKR_EXEC_ERROR("RasterPass '{}' source {} has no color attachment",
                     std::string(passName), sourceIndex);
    }

    const auto &color = source.color();
    if (!color.hasSampler()) {
      VKR_EXEC_ERROR("RasterPass '{}' source {} color has no sampler",
                     std::string(passName), sourceIndex);
    }

    imageInfo.imageLayout = imageLayoutForAttachment(color);
    imageInfo.imageView = color.imageView();
    imageInfo.sampler = color.sampler();
    break;
  }

  case RenderPassInputKind::Depth: {
    const auto *depth = source.depth();
    if (depth == nullptr) {
      VKR_EXEC_ERROR("RasterPass '{}' source {} has no depth attachment",
                     std::string(passName), sourceIndex);
    }

    if (!depth->hasSampler()) {
      VKR_EXEC_ERROR("RasterPass '{}' source {} depth has no sampler",
                     std::string(passName), sourceIndex);
    }

    imageInfo.imageLayout =
        depth->desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            : depth->desc().finalLayout;
    imageInfo.imageView = depth->imageView();
    imageInfo.sampler = depth->sampler();
    break;
  }
  }

  return imageInfo;
}

} // namespace

RasterPass::RasterPass(RenderExecutor &executor, const core::Device &device,
                       const core::CommandPool &commandPool,
                       scene::Scene &scene)
    : executor_(executor), device_(device), command_pool_(commandPool),
      scene_(scene), descriptor_pool_(device), descriptor_layout_(device),
      descriptor_sets_(device, descriptor_pool_, descriptor_layout_) {}

RasterPass::~RasterPass() { destroy(); }

void RasterPass::create() {
  validate(desc_);
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void RasterPass::destroy() noexcept {
  pipeline_.reset();
  descriptor_sets_.destroy();
  descriptor_layout_.destroy();
  descriptor_pool_.destroy();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void RasterPass::update(const RasterPassDesc &desc) {
  ensureConfigurable();
  if (target_ || render_pass_ || pipeline_ || descriptor_layout_.valid()) {
    VKR_EXEC_ERROR("RasterPass '{}' must be destroyed before updating its "
                   "configuration",
                   name());
  }
  validate(desc);
  auto nextDesc = desc;
  desc_ = std::move(nextDesc);
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
  executor_.beginPass(*framebuffers_, desc_.clearValues);
  executor_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    descriptor_sets_.write(executor_.frameIndex());
    if (descriptor_sets_.valid()) {
      executor_.bindPipeline(*pipeline_, descriptor_sets_);
    } else {
      executor_.bindPipeline(*pipeline_);
    }
    if (desc_.meshNames.empty()) {
      executor_.drawGeometry();
    } else {
      for (const auto &meshName : desc_.meshNames) {
        const auto &mesh = scene_.mesh(meshName);
        if (!mesh.isValid()) {
          VKR_EXEC_ERROR("RasterPass '{}' mesh resource not found: {}", name(),
                         meshName);
        }

        const auto vertexBuffer = mesh.vertexBufferBase();
        const auto indexBuffer = mesh.indexBuffer();
        if (!vertexBuffer || !indexBuffer) {
          VKR_EXEC_ERROR("RasterPass '{}' mesh '{}' has invalid buffers",
                         name(), meshName);
        }

        executor_.drawIndexed(vertexBuffer->get(), indexBuffer->get());
      }
    }
  }

  executor_.endPass();
  executor_.endProfileScope();
}

auto RasterPass::target() -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

auto RasterPass::target() const -> const OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

void RasterPass::createTarget() {
  target_ = std::make_unique<OffscreenTarget>(device_, command_pool_);
  target_->update(desc_.target);
}

void RasterPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  pipeline::RenderPassDesc renderPassDesc{};

  if (target_->hasColor()) {
    renderPassDesc = pipeline::RenderPassDesc::makeOffscreen(
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
    renderPassDesc = pipeline::RenderPassDesc::makeDepthOnly(
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
  auto framebufferDesc = FramebufferDesc::single(
      target_->width(), target_->height(), target_->attachmentViews());

  framebuffers_ = std::make_unique<FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void RasterPass::validate(const RasterPassDesc &desc) const {
  if (!desc.target.isValid()) {
    VKR_EXEC_ERROR("RasterPass '{}' has an invalid target descriptor",
                   name());
  }
  if (!desc.pipeline.isValid()) {
    VKR_EXEC_ERROR("RasterPass '{}' has an invalid pipeline descriptor",
                   name());
  }
  pipeline::DescriptorSetLayoutDesc layoutDesc{
      .bindings = desc.descriptorBindings};
  for (const auto &input : desc.inputs) {
    layoutDesc.bindings.push_back(
        {.layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    input.stageFlags}});
  }
  if (!layoutDesc.isValid()) {
    VKR_EXEC_ERROR("RasterPass '{}' requires unique bindings with nonzero "
                   "counts/stages",
                   name());
  }
  for (const auto &binding : layoutDesc.bindings) {
    const auto &layout = binding.layout;
    if (layout.descriptorCount != 1 ||
        (layout.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
      VKR_EXEC_ERROR("RasterPass '{}' has an unsupported descriptor binding {}",
                     name(), layout.binding);
    }
  }
}

void RasterPass::createDescriptors() {
  if (desc_.descriptorBindings.empty() && desc_.inputs.empty()) {
    descriptor_sets_.update({.setCount = 0});
    return;
  }

  if (desc_.inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR("RasterPass '{}' input count mismatch: desc={} sources={}",
                   name(), desc_.inputs.size(), sources_.size());
  }

  std::vector<pipeline::DescriptorBinding> bindings = desc_.descriptorBindings;
  bindings.reserve(desc_.descriptorBindings.size() + desc_.inputs.size());

  for (size_t index = 0; index < desc_.inputs.size(); ++index) {
    const auto &input = desc_.inputs[index];
    bindings.push_back(pipeline::DescriptorBinding{
        .layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   input.stageFlags}});
  }

  const pipeline::DescriptorSetLayoutDesc layoutDesc{.bindings = bindings};
  descriptor_pool_.update(pipeline::DescriptorPoolDesc::sets(
      layoutDesc, executor_.framesInFlight()));
  descriptor_layout_.update(layoutDesc);

  descriptor_sets_.update({.setCount = executor_.framesInFlight()});
  descriptor_sets_.write(createDescriptorWrites());
  for (uint32_t index = 0; index < descriptor_sets_.count(); ++index) {
    descriptor_sets_.write(index);
  }
}

void RasterPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_.valid() ? descriptor_layout_.layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  pipeline_ =
      std::make_unique<pipeline::GraphicsPipeline>(device_, *render_pass_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("RasterPass '{}' failed to create graphics pipeline '{}'",
                   name(), pipelineDesc.name);
  }
}

auto RasterPass::createDescriptorWrites() const
    -> std::vector<pipeline::DescriptorSetWrite> {
  const uint32_t frameCount = executor_.framesInFlight();
  std::vector<pipeline::DescriptorSetWrite> writes{};
  writes.reserve(frameCount);
  for (uint32_t frame = 0; frame < frameCount; ++frame) {
    writes.push_back(pipeline::DescriptorSetWrite::forSet(frame));
  }

  for (size_t sourceIndex = 0; sourceIndex < sources_.size(); ++sourceIndex) {
    const auto &input = desc_.inputs[sourceIndex];
    const auto source =
        sources_[sourceIndex].get().capability<RenderTargetCapability>();
    if (!source) {
      VKR_EXEC_ERROR("RasterPass '{}' source '{}' has no render target", name(),
                     sources_[sourceIndex].get().name());
    }
    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
      const VkDescriptorImageInfo imageInfo = sourceImageInfo(
          name(), sourceIndex, source->get().target(frameIndex), input);
      writes[frameIndex].images.push_back(pipeline::DescriptorImageWrite::one(
          input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo));
    }
  }

  return writes;
}

} // namespace vkr::exec
