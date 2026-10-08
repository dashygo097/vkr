#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string_view>

namespace vkr::exec {
namespace {

auto imageLayoutForColor(const pipeline::ColorAttachment &color)
    -> VkImageLayout {
  return color.desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
             ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             : color.desc().finalLayout;
}

auto sourceImageInfo(std::string_view passName, size_t sourceIndex,
                     const pipeline::OffscreenTarget &source,
                     const RenderPassInputDesc &input)
    -> VkDescriptorImageInfo {
  VkDescriptorImageInfo imageInfo{};

  switch (input.kind) {
  case RenderPassInputKind::Color: {
    if (!source.hasColor()) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source {} has no color "
                     "attachment",
                     std::string(passName), sourceIndex);
    }

    const auto &color = source.color();
    if (!color.hasSampler()) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source {} color has no "
                     "sampler",
                     std::string(passName), sourceIndex);
    }

    imageInfo.imageLayout = imageLayoutForColor(color);
    imageInfo.imageView = color.imageView();
    imageInfo.sampler = color.sampler();
    break;
  }

  case RenderPassInputKind::Depth: {
    const auto *depth = source.depth();
    if (depth == nullptr) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source {} has no depth "
                     "attachment",
                     std::string(passName), sourceIndex);
    }

    if (!depth->hasSampler()) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source {} depth has no "
                     "sampler",
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

FeedbackFullscreenPass::FeedbackFullscreenPass(
    RenderExecutor &executor, const core::Device &device,
    std::vector<std::reference_wrapper<Pass>> sources)
    : executor_(executor), device_(device), sources_(std::move(sources)),
      descriptor_pool_(device), descriptor_layout_(device),
      descriptor_sets_(device, descriptor_pool_, descriptor_layout_) {}

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
  descriptor_sets_.destroy();
  descriptor_layout_.destroy();
  descriptor_pool_.destroy();
  framebuffers_.clear();
  render_pass_.reset();
  target_.reset();
}

void FeedbackFullscreenPass::update(const FeedbackFullscreenPassDesc &desc) {
  ensureConfigurable();
  if (target_ || render_pass_ || pipeline_ || descriptor_layout_.valid()) {
    VKR_EXEC_ERROR(
        "FeedbackFullscreenPass '{}' must be destroyed before updating its "
        "configuration",
        name());
  }
  validate(desc);
  auto nextDesc = desc;
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
    descriptor_sets_.write(executor_.frameIndex());
    if (descriptor_sets_.valid()) {
      executor_.bindPipeline(*pipeline_, descriptor_sets_);
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
  pipeline::DescriptorSetLayoutDesc layoutDesc{.bindings =
                                                   desc.descriptorBindings};
  if (desc.historyInput) {
    layoutDesc.bindings.push_back(
        {.layout = {desc.historyInput->binding,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    desc.historyInput->stageFlags}});
  }
  for (const auto &input : desc.inputs) {
    layoutDesc.bindings.push_back(
        {.layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    input.stageFlags}});
  }
  if (!layoutDesc.isValid()) {
    VKR_EXEC_ERROR(
        "FeedbackFullscreenPass '{}' requires unique bindings with nonzero "
        "counts/stages",
        name());
  }
  for (const auto &binding : layoutDesc.bindings) {
    const auto &layout = binding.layout;
    if (layout.descriptorCount != 1 ||
        (layout.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' has an unsupported "
                     "descriptor binding {}",
                     name(), layout.binding);
    }
  }
}

void FeedbackFullscreenPass::createDescriptors() {
  const auto inputs = resolvedInputs();
  if (inputs.empty() && !desc_.historyInput &&
      desc_.descriptorBindings.empty()) {
    descriptor_sets_.update({.setCount = 0});
    return;
  }

  std::vector<pipeline::DescriptorBinding> bindings = desc_.descriptorBindings;
  bindings.reserve(desc_.descriptorBindings.size() + inputs.size() +
                   (desc_.historyInput ? 1U : 0U));

  if (desc_.historyInput) {
    bindings.push_back(pipeline::DescriptorBinding{
        .layout = {desc_.historyInput->binding,
                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   desc_.historyInput->stageFlags}});
  }

  for (const auto &input : inputs) {
    bindings.push_back(pipeline::DescriptorBinding{
        .layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   input.stageFlags}});
  }

  const pipeline::DescriptorSetLayoutDesc layoutDesc{.bindings = bindings};
  descriptor_pool_.update(pipeline::DescriptorPoolDesc::sets(
      layoutDesc, executor_.framesInFlight()));
  descriptor_layout_.update(layoutDesc);

  descriptor_sets_.update({.setCount = executor_.framesInFlight()});
  descriptor_sets_.write(createDescriptorWrites(inputs));
  for (uint32_t index = 0; index < descriptor_sets_.count(); ++index) {
    descriptor_sets_.write(index);
  }
}

void FeedbackFullscreenPass::createPipeline() {
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
    for (const auto &binding : desc_.descriptorBindings) {
      firstBinding = std::max(firstBinding, binding.layout.binding + 1U);
    }

    if (desc_.historyInput) {
      firstBinding = std::max(firstBinding, desc_.historyInput->binding + 1U);
    }

    for (uint32_t index = 0; index < sources_.size(); ++index) {
      inputs.push_back(RenderPassInputDesc{.binding = firstBinding + index});
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

auto FeedbackFullscreenPass::createDescriptorWrites(
    const std::vector<RenderPassInputDesc> &inputs)
    -> std::vector<pipeline::DescriptorSetWrite> {
  std::vector<pipeline::DescriptorSetWrite> writes{};
  const uint32_t frameCount = executor_.framesInFlight();
  writes.reserve(frameCount);

  for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWrite::forSet(frameIndex));
  }

  if (desc_.historyInput) {
    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
      const VkDescriptorImageInfo imageInfo = sourceImageInfo(
          name(), 0, historyTarget(frameIndex), *desc_.historyInput);

      writes[frameIndex].images.push_back(pipeline::DescriptorImageWrite::one(
          desc_.historyInput->binding,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo));
    }
  }

  for (size_t index = 0; index < sources_.size(); ++index) {
    const auto source =
        sources_[index].get().capability<RenderTargetCapability>();
    if (!source) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source '{}' has no render "
                     "target",
                     name(), sources_[index].get().name());
    }
    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
      const VkDescriptorImageInfo imageInfo = sourceImageInfo(
          name(), index, source->get().target(frameIndex), inputs[index]);

      writes[frameIndex].images.push_back(pipeline::DescriptorImageWrite::one(
          inputs[index].binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          imageInfo));
    }
  }

  return writes;
}

} // namespace vkr::exec
