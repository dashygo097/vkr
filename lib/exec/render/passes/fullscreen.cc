#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/uniform_buffer.hh"
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
          "FullscreenPass '{}' source {} needs a sampled color attachment",
          std::string(passName), sourceIndex);
    }
    set.texture(input.binding, source.color());
    break;
  }
  case RenderPassInputKind::Depth: {
    const auto *depth = source.depth();
    if (depth == nullptr || !depth->hasSampler()) {
      VKR_EXEC_ERROR(
          "FullscreenPass '{}' source {} needs a sampled depth attachment",
          std::string(passName), sourceIndex);
    }
    set.texture(input.binding, *depth);
    break;
  }
  }
}

} // namespace

FullscreenPass::FullscreenPass(
    RenderExecutor &executor, const core::Device &device,
    std::vector<std::reference_wrapper<Pass>> sources)
    : executor_(executor), device_(device), sources_(std::move(sources)),
      descriptor_pool_(device) {
  descriptor_sets_.resize(executor_.framesInFlight());
}

FullscreenPass::~FullscreenPass() { destroy(); }

auto FullscreenPass::uniform(uint32_t setIndex, uint32_t binding,
                             const res::Buffer &buffer) -> FullscreenPass & {
  return uniform(setIndex, binding, buffer.slice(0, buffer.size()));
}

auto FullscreenPass::uniform(uint32_t setIndex, uint32_t binding,
                             const res::BufferSlice &slice)
    -> FullscreenPass & {
  for (auto &frame : descriptor_sets_) {
    frame.at(setIndex).uniform(binding, slice);
  }
  return *this;
}

auto FullscreenPass::uniform(uint32_t setIndex, uint32_t binding,
                             const std::vector<res::UniformBuffer> &buffers)
    -> FullscreenPass & {
  if (buffers.size() < descriptor_sets_.size()) {
    VKR_EXEC_ERROR(
        "FullscreenPass '{}' requires a uniform buffer for every frame",
        name());
  }
  for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
    descriptor_sets_[frame].at(setIndex).uniform(binding, buffers[frame]);
  }
  return *this;
}

auto FullscreenPass::texture(uint32_t setIndex, uint32_t binding,
                             const res::Texture &texture) -> FullscreenPass & {
  for (auto &frame : descriptor_sets_) {
    frame.at(setIndex).texture(binding, texture);
  }
  return *this;
}

auto FullscreenPass::texture(uint32_t setIndex, uint32_t binding,
                             const res::Cubemap &cubemap) -> FullscreenPass & {
  for (auto &frame : descriptor_sets_) {
    frame.at(setIndex).texture(binding, cubemap);
  }
  return *this;
}

auto FullscreenPass::texture(uint32_t setIndex, uint32_t binding,
                             const pipe::ColorAttachment &attachment)
    -> FullscreenPass & {
  for (auto &frame : descriptor_sets_) {
    frame.at(setIndex).texture(binding, attachment);
  }
  return *this;
}

auto FullscreenPass::texture(uint32_t setIndex, uint32_t binding,
                             const pipe::DepthAttachment &attachment)
    -> FullscreenPass & {
  for (auto &frame : descriptor_sets_) {
    frame.at(setIndex).texture(binding, attachment);
  }
  return *this;
}

auto FullscreenPass::storage(uint32_t setIndex, uint32_t binding,
                             const res::Buffer &buffer) -> FullscreenPass & {
  for (auto &frame : descriptor_sets_) {
    frame.at(setIndex).storage(binding, buffer);
  }
  return *this;
}

auto FullscreenPass::storage(uint32_t setIndex, uint32_t binding,
                             const res::BufferSlice &slice)
    -> FullscreenPass & {
  for (auto &frame : descriptor_sets_) {
    frame.at(setIndex).storage(binding, slice);
  }
  return *this;
}

void FullscreenPass::create() {
  validate(desc_);
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void FullscreenPass::destroy() noexcept {
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

void FullscreenPass::update(const FullscreenPassDesc &desc) {
  ensureConfigurable();
  if (target_ || render_pass_ || pipeline_ || descriptor_pool_.valid()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' must be destroyed before updating its "
                   "configuration",
                   name());
  }
  validate(desc);
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

void FullscreenPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' recorded before create", name());
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
    executor_.drawFullscreenTriangle();
  }

  executor_.endPass();
  executor_.endProfileScope();
}

auto FullscreenPass::dependencies() const
    -> std::vector<std::reference_wrapper<const Pass>> {
  return {sources_.begin(), sources_.end()};
}

auto FullscreenPass::addSource(Pass &source) -> FullscreenPass & {
  ensureConfigurable();
  sources_.emplace_back(source);
  return *this;
}

auto FullscreenPass::setSources(
    std::vector<std::reference_wrapper<Pass>> sources) -> FullscreenPass & {
  ensureConfigurable();
  sources_ = std::move(sources);
  return *this;
}

auto FullscreenPass::target() -> pipe::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' target requested before create",
                   name());
  }

  return *target_;
}

auto FullscreenPass::target() const -> const pipe::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' target requested before create",
                   name());
  }

  return *target_;
}

void FullscreenPass::createTarget() {
  target_ = std::make_unique<pipe::OffscreenTarget>(device_);
  target_->update(desc_.target);
}

void FullscreenPass::createRenderPass() {
  render_pass_ = std::make_unique<pipe::RenderPass>(device_);
  pipe::RenderPassDesc renderPassDesc{};

  if (target_->hasColor()) {
    renderPassDesc = pipe::RenderPassDesc::makeOffscreen(
        target_->color().desc().format, target_->depth()
                                            ? target_->depth()->desc().format
                                            : VK_FORMAT_UNDEFINED);

    const auto &colorDesc = target_->color().desc();
    if (colorDesc.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      renderPassDesc.colors[0].finalLayout = colorDesc.finalLayout;
    } else if ((colorDesc.usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) == 0) {
      renderPassDesc.colors[0].finalLayout =
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  } else if (target_->depth()) {
    renderPassDesc =
        pipe::RenderPassDesc::makeDepthOnly(target_->depth()->desc().format);
  } else {
    VKR_EXEC_ERROR("FullscreenPass '{}' target has no attachments", name());
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

void FullscreenPass::createFramebuffers() {
  auto framebufferDesc = FramebuffersDesc::single(
      target_->width(), target_->height(), target_->attachmentViews());

  framebuffers_ = std::make_unique<Framebuffers>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void FullscreenPass::validate(const FullscreenPassDesc &desc) const {
  if (!desc.target.isValid()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' has an invalid target descriptor",
                   name());
  }
  if (!desc.pipeline.isValid()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' has an invalid pipeline descriptor",
                   name());
  }
  const auto &properties = device_.properties();
  auto layouts = desc.descriptorLayouts;
  if (layouts.size() > properties.limits.maxBoundDescriptorSets) {
    VKR_EXEC_ERROR("FullscreenPass '{}' exceeds maxBoundDescriptorSets",
                   name());
  }
  for (const auto &input : desc.inputs) {
    if (input.setIndex >= properties.limits.maxBoundDescriptorSets) {
      VKR_EXEC_ERROR("FullscreenPass '{}' input set index is out of range",
                     name());
    }
    if (input.setIndex >= layouts.size()) {
      layouts.resize(static_cast<size_t>(input.setIndex) + 1);
    }
    layouts[input.setIndex].texture(input.binding, input.stageFlags);
  }
  for (const auto &layoutDesc : layouts) {
    if (!layoutDesc.isValid()) {
      VKR_EXEC_ERROR(
          "FullscreenPass '{}' requires unique bindings within each set "
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
            "FullscreenPass '{}' has an unsupported descriptor binding {}",
            name(), layout.binding);
      }
    }
  }
}

void FullscreenPass::createDescriptors() {
  const auto inputs = resolvedInputs();
  if (inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' input count mismatch", name());
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
      VKR_EXEC_ERROR("FullscreenPass '{}' source '{}' has no render target",
                     name(), sources_[index].get().name());
    }
    for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
      bindSource(descriptor_sets_[frame].at(inputs[index].setIndex), name(),
                 index, source->get().target(frame), inputs[index]);
    }
  }
}

void FullscreenPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  if (pipelineDesc.layout.setLayouts.empty()) {
    for (const auto &layout : descriptor_layouts_) {
      if (layout->valid()) {
        pipelineDesc.layout.setLayouts.push_back(layout->layout());
      }
    }
  }

  pipeline_ = std::make_unique<pipe::GraphicsPipeline>(device_, *render_pass_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR(
        "FullscreenPass '{}' failed to create graphics pipeline '{}'", name(),
        pipelineDesc.name);
  }
}

auto FullscreenPass::resolvedInputs() const
    -> std::vector<RenderPassInputDesc> {
  if (desc_.inputs.empty()) {
    std::vector<RenderPassInputDesc> inputs{};
    inputs.reserve(sources_.size());

    uint32_t firstBinding = 0;
    if (!desc_.descriptorLayouts.empty()) {
      for (const auto &binding : desc_.descriptorLayouts[0].bindings) {
        firstBinding = std::max(firstBinding, binding.layout.binding + 1U);
      }
    }

    for (uint32_t index = 0; index < sources_.size(); ++index) {
      inputs.push_back(
          RenderPassInputDesc{.setIndex = 0, .binding = firstBinding + index});
    }

    return inputs;
  }

  if (desc_.inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR(
        "FullscreenPass '{}' input count mismatch: desc={} sources={}", name(),
        desc_.inputs.size(), sources_.size());
  }

  return desc_.inputs;
}

} // namespace vkr::exec
