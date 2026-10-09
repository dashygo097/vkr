#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string_view>

namespace vkr::exec {
namespace {

void bindSource(pipeline::DescriptorSet &set, std::string_view passName,
                size_t sourceIndex, const pipeline::OffscreenTarget &source,
                const RenderPassInputDesc &input) {
  switch (input.kind) {
  case RenderPassInputKind::Color: {
    if (!source.hasColor() || !source.color().hasSampler()) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source {} needs a sampled "
                     "color attachment",
                     std::string(passName), sourceIndex);
    }
    set.texture(input.binding, source.color());
    break;
  }
  case RenderPassInputKind::Depth: {
    const auto *depth = source.depth();
    if (depth == nullptr || !depth->hasSampler()) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source {} needs a sampled "
                     "depth attachment",
                     std::string(passName), sourceIndex);
    }
    set.texture(input.binding, *depth);
    break;
  }
  }
}

} // namespace

FeedbackFullscreenPass::FeedbackFullscreenPass(
    RenderExecutor &executor, const core::Device &device,
    std::vector<std::reference_wrapper<Pass>> sources)
    : executor_(executor), device_(device), sources_(std::move(sources)),
      descriptor_pool_(device) {
  descriptor_sets_.resize(executor_.framesInFlight());
}

FeedbackFullscreenPass::~FeedbackFullscreenPass() { destroy(); }

void FeedbackFullscreenPass::create() {
  validate(desc_);
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void FeedbackFullscreenPass::destroy() noexcept {
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
  framebuffers_.clear();
  render_pass_.reset();
  target_.reset();
}

void FeedbackFullscreenPass::update(const FeedbackFullscreenPassDesc &desc) {
  ensureConfigurable();
  if (target_ || render_pass_ || pipeline_ || descriptor_pool_.valid()) {
    VKR_EXEC_ERROR(
        "FeedbackFullscreenPass '{}' must be destroyed before updating its "
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
  if (nextDesc.historyInput &&
      nextDesc.historyInput->setIndex >= nextDesc.descriptorLayouts.size()) {
    nextDesc.descriptorLayouts.resize(
        static_cast<size_t>(nextDesc.historyInput->setIndex) + 1);
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
        std::make_unique<pipeline::DescriptorSetLayout>(device_));
    for (auto &frame : descriptor_sets_) {
      frame.emplace_back(device_, descriptor_pool_,
                         *descriptor_layouts_.back());
    }
  }
  desc_ = std::move(nextDesc);
}

void FeedbackFullscreenPass::record() {
  if (!target_ || !render_pass_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' recorded before create",
                   name());
  }

  const uint32_t frameIndex = executor_.frameIndex();
  const uint32_t writeIndex = target_->writeIndexForFrame(frameIndex);
  auto &writeTarget = target_->writeForFrame(frameIndex);
  auto &framebuffer = framebuffers_[writeIndex];

  if (!framebuffer) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' has no framebuffer for "
                   "target index {}",
                   name(), writeIndex);
  }

  executor_.beginProfileScope(name());
  executor_.beginPass(*render_pass_, *framebuffer, desc_.clearValues);
  executor_.setViewportAndScissor({writeTarget.width(), writeTarget.height()});

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

auto FeedbackFullscreenPass::addSource(Pass &source)
    -> FeedbackFullscreenPass & {
  ensureConfigurable();
  sources_.emplace_back(source);
  return *this;
}

auto FeedbackFullscreenPass::setSources(
    std::vector<std::reference_wrapper<Pass>> sources)
    -> FeedbackFullscreenPass & {
  ensureConfigurable();
  sources_ = std::move(sources);
  return *this;
}

auto FeedbackFullscreenPass::target() -> pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::target() const
    -> const pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::target(uint32_t frameIndex)
    -> pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(frameIndex);
}

auto FeedbackFullscreenPass::target(uint32_t frameIndex) const
    -> const pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(frameIndex);
}

auto FeedbackFullscreenPass::historyTarget() -> pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target requested "
                   "before create",
                   name());
  }

  return target_->readForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::historyTarget() const
    -> const pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target requested "
                   "before create",
                   name());
  }

  return target_->readForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::historyTarget(uint32_t frameIndex)
    -> pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target requested "
                   "before create",
                   name());
  }

  return target_->readForFrame(frameIndex);
}

auto FeedbackFullscreenPass::historyTarget(uint32_t frameIndex) const
    -> const pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target requested "
                   "before create",
                   name());
  }

  return target_->readForFrame(frameIndex);
}

void FeedbackFullscreenPass::createTarget() {
  auto targetDesc = desc_.target;
  targetDesc.frameCount = executor_.framesInFlight();

  target_ = std::make_unique<pipeline::FrameHistoryTarget>(device_);
  target_->update(targetDesc);
}

void FeedbackFullscreenPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  const auto &writeTarget = target_->writeForFrame(0);
  pipeline::RenderPassDesc renderPassDesc{};

  if (writeTarget.hasColor()) {
    renderPassDesc = pipeline::RenderPassDesc::makeOffscreen(
        writeTarget.color().desc().format,
        writeTarget.depth() ? writeTarget.depth()->desc().format
                            : VK_FORMAT_UNDEFINED);

    const auto &colorDesc = writeTarget.color().desc();
    if (colorDesc.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      renderPassDesc.colors[0].finalLayout = colorDesc.finalLayout;
    } else if ((colorDesc.usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) == 0) {
      renderPassDesc.colors[0].finalLayout =
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  } else if (writeTarget.depth()) {
    renderPassDesc = pipeline::RenderPassDesc::makeDepthOnly(
        writeTarget.depth()->desc().format);
  } else {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target has no attachments",
                   name());
  }

  if (writeTarget.depth()) {
    const auto &depthDesc = writeTarget.depth()->desc();
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

void FeedbackFullscreenPass::createFramebuffers() {
  framebuffers_.clear();
  framebuffers_.resize(target_->targetCount());

  for (uint32_t index = 0; index < framebuffers_.size(); ++index) {
    auto &offscreen = target_->target(index);
    auto framebufferDesc = FramebuffersDesc::single(
        offscreen.width(), offscreen.height(), offscreen.attachmentViews());

    framebuffers_[index] =
        std::make_unique<Framebuffers>(device_, *render_pass_);
    framebuffers_[index]->update(framebufferDesc);
  }
}

void FeedbackFullscreenPass::validate(
    const FeedbackFullscreenPassDesc &desc) const {
  if (!desc.target.target.isValid()) {
    VKR_EXEC_ERROR(
        "FeedbackFullscreenPass '{}' has an invalid target descriptor", name());
  }
  if (!desc.pipeline.isValid()) {
    VKR_EXEC_ERROR(
        "FeedbackFullscreenPass '{}' has an invalid pipeline descriptor",
        name());
  }
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.physicalDevice(), &properties);
  auto layouts = desc.descriptorLayouts;
  if (layouts.size() > properties.limits.maxBoundDescriptorSets) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' exceeds maxBoundDescriptorSets",
                   name());
  }
  for (const auto &input : desc.inputs) {
    if (input.setIndex >= properties.limits.maxBoundDescriptorSets) {
      VKR_EXEC_ERROR(
          "FeedbackFullscreenPass '{}' input set index is out of range",
          name());
    }
    if (input.setIndex >= layouts.size()) {
      layouts.resize(static_cast<size_t>(input.setIndex) + 1);
    }
    layouts[input.setIndex].bindings.push_back(
        {.layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    input.stageFlags}});
  }
  if (desc.historyInput) {
    const auto &input = *desc.historyInput;
    if (input.setIndex >= properties.limits.maxBoundDescriptorSets) {
      VKR_EXEC_ERROR(
          "FeedbackFullscreenPass '{}' input set index is out of range",
          name());
    }
    if (input.setIndex >= layouts.size()) {
      layouts.resize(static_cast<size_t>(input.setIndex) + 1);
    }
    layouts[input.setIndex].bindings.push_back(
        {.layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    input.stageFlags}});
  }
  for (const auto &layoutDesc : layouts) {
    if (!layoutDesc.isValid()) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' requires unique bindings "
                     "within each set "
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
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' has an unsupported "
                       "descriptor binding {}",
                       name(), layout.binding);
      }
    }
  }
}

void FeedbackFullscreenPass::createDescriptors() {
  const auto inputs = resolvedInputs();
  if (inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' input count mismatch", name());
  }
  auto layouts = desc_.descriptorLayouts;
  for (const auto &input : inputs) {
    if (input.setIndex >= layouts.size()) {
      layouts.resize(static_cast<size_t>(input.setIndex) + 1);
    }
    layouts[input.setIndex].bindings.push_back(
        {.layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    input.stageFlags}});
  }
  if (desc_.historyInput) {
    const auto &input = *desc_.historyInput;
    if (input.setIndex >= layouts.size()) {
      layouts.resize(static_cast<size_t>(input.setIndex) + 1);
    }
    layouts[input.setIndex].bindings.push_back(
        {.layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    input.stageFlags}});
  }
  if (layouts.empty()) {
    return;
  }
  descriptor_pool_.update(
      pipeline::DescriptorPoolDesc::sets(layouts, executor_.framesInFlight()));
  for (uint32_t setIndex = 0; setIndex < layouts.size(); ++setIndex) {
    descriptor_layouts_[setIndex]->update(layouts[setIndex]);
    for (auto &frame : descriptor_sets_) {
      frame[setIndex].create();
    }
  }
  if (desc_.historyInput) {
    for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
      bindSource(descriptor_sets_[frame].at(desc_.historyInput->setIndex),
                 name(), 0, historyTarget(frame), *desc_.historyInput);
    }
  }
  for (size_t index = 0; index < sources_.size(); ++index) {
    const auto source =
        sources_[index].get().capability<RenderTargetCapability>();
    if (!source) {
      VKR_EXEC_ERROR(
          "FeedbackFullscreenPass '{}' source '{}' has no render target",
          name(), sources_[index].get().name());
    }
    for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
      bindSource(descriptor_sets_[frame].at(inputs[index].setIndex), name(),
                 index, source->get().target(frame), inputs[index]);
    }
  }
}

void FeedbackFullscreenPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  if (pipelineDesc.layout.setLayouts.empty()) {
    for (const auto &layout : descriptor_layouts_) {
      if (layout->valid()) {
        pipelineDesc.layout.setLayouts.push_back(layout->layout());
      }
    }
  }

  pipeline_ =
      std::make_unique<pipeline::GraphicsPipeline>(device_, *render_pass_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' failed to create graphics "
                   "pipeline '{}'",
                   name(), pipelineDesc.name);
  }
}

auto FeedbackFullscreenPass::resolvedInputs() const
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

    if (desc_.historyInput && desc_.historyInput->setIndex == 0) {
      firstBinding = std::max(firstBinding, desc_.historyInput->binding + 1U);
    }

    for (uint32_t index = 0; index < sources_.size(); ++index) {
      inputs.push_back(
          RenderPassInputDesc{.setIndex = 0, .binding = firstBinding + index});
    }

    return inputs;
  }

  if (desc_.inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' input count mismatch: "
                   "desc={} sources={}",
                   name(), desc_.inputs.size(), sources_.size());
  }

  return desc_.inputs;
}

} // namespace vkr::exec
