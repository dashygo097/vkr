#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/logger.hh"
#include <algorithm>

namespace vkr::exec {
namespace {

auto imageLayoutForColor(const ColorAttachment &color) -> VkImageLayout {
  return color.desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
             ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             : color.desc().finalLayout;
}

auto defaultFeedbackDescriptorPoolDesc(
    pipeline::DescriptorPoolDesc poolDesc,
    const std::vector<pipeline::DescriptorBinding> &resourceBindings,
    bool hasHistoryInput, size_t sourceCount, uint32_t frameCount)
    -> pipeline::DescriptorPoolDesc {
  if (poolDesc.maxSets != 0) {
    return poolDesc;
  }

  uint32_t uniformCount = 0;
  uint32_t imageCount =
      static_cast<uint32_t>(sourceCount + (hasHistoryInput ? 1U : 0U));

  for (const auto &binding : resourceBindings) {
    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      uniformCount++;
      break;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      imageCount++;
      break;
    default:
      break;
    }
  }

  if (uniformCount > 0) {
    poolDesc.poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         static_cast<uint32_t>(frameCount * uniformCount)});
  }

  if (imageCount > 0) {
    poolDesc.poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         static_cast<uint32_t>(frameCount * imageCount)});
  }

  poolDesc.maxSets = frameCount;
  return poolDesc;
}

auto nextBindingAfter(
    const std::vector<pipeline::DescriptorBinding> &resourceBindings,
    const std::optional<FullscreenPassInputDesc> &historyInput) -> uint32_t {
  uint32_t nextBinding = 0;

  for (const auto &binding : resourceBindings) {
    const uint32_t descriptorCount = binding.layout.descriptorCount == 0
                                         ? 1U
                                         : binding.layout.descriptorCount;
    nextBinding =
        std::max(nextBinding, binding.layout.binding + descriptorCount);
  }

  if (historyInput) {
    nextBinding = std::max(nextBinding, historyInput->binding + 1U);
  }

  return nextBinding;
}

void appendResourceDescriptorWrites(
    std::string_view passName, const scene::Scene *scene,
    const std::vector<pipeline::DescriptorBinding> &bindings,
    std::vector<pipeline::DescriptorSetWriteDesc> &writes,
    uint32_t frameCount) {
  if (bindings.empty()) {
    return;
  }

  if (scene == nullptr) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' has descriptor resource "
                   "bindings but no Scene",
                   std::string(passName));
  }

  for (const auto &binding : bindings) {
    if (binding.name.empty()) {
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' descriptor binding {} has "
                     "empty resource name",
                     std::string(passName), binding.layout.binding);
    }

    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = scene->getUniformBuffer(binding.name);
      if (!uniformBuffer) {
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' uniform buffer resource "
                       "not found: {}",
                       std::string(passName), binding.name);
      }

      if (uniformBuffer->frameCount() != frameCount) {
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' uniform buffer '{}' "
                       "frame count mismatch: {} vs {}",
                       std::string(passName), binding.name,
                       uniformBuffer->frameCount(), frameCount);
      }

      for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const auto bufferInfo = uniformBuffer->descriptorInfo(frameIndex);

        writes[frameIndex].buffers.push_back(
            pipeline::DescriptorBufferWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                bufferInfo));
      }
      break;
    }

    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      auto texture = scene->getTexture(binding.name);
      if (!texture) {
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' texture resource not "
                       "found: {}",
                       std::string(passName), binding.name);
      }

      if (!texture->hasSampler()) {
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' texture sampler not "
                       "found: {}",
                       std::string(passName), binding.name);
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = texture->layout();
      imageInfo.imageView = texture->imageView();
      imageInfo.sampler = texture->sampler();

      for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        writes[frameIndex].images.push_back(
            pipeline::DescriptorImageWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                imageInfo));
      }
      break;
    }

    default:
      VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' cannot create descriptor "
                     "writes for resource '{}' with descriptor type {}",
                     std::string(passName), binding.name,
                     static_cast<int>(binding.layout.descriptorType));
    }
  }
}

void validateUniqueDescriptorBindings(
    std::string_view passName,
    const std::vector<pipeline::DescriptorBinding> &bindings) {
  for (size_t i = 0; i < bindings.size(); ++i) {
    for (size_t j = i + 1; j < bindings.size(); ++j) {
      if (bindings[i].layout.binding == bindings[j].layout.binding) {
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' has duplicate "
                       "descriptor binding {} for '{}' and '{}'",
                       std::string(passName), bindings[i].layout.binding,
                       bindings[i].name, bindings[j].name);
      }
    }
  }
}

} // namespace

FeedbackFullscreenPass::FeedbackFullscreenPass(
    Executor &executor, const core::Device &device,
    const core::CommandPool &commandPool,
    std::vector<FullscreenPassSource> sources)
    : executor_(executor), device_(device), command_pool_(commandPool),
      sources_(std::move(sources)) {}

FeedbackFullscreenPass::FeedbackFullscreenPass(
    Executor &executor, const core::Device &device,
    const core::CommandPool &commandPool, scene::Scene &scene,
    std::vector<FullscreenPassSource> sources)
    : executor_(executor), device_(device), command_pool_(commandPool),
      scene_(&scene), sources_(std::move(sources)) {}

FeedbackFullscreenPass::~FeedbackFullscreenPass() { destroy(); }

void FeedbackFullscreenPass::create() {
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void FeedbackFullscreenPass::destroy() {
  pipeline_.reset();
  descriptor_sets_.reset();
  descriptor_layout_.reset();
  descriptor_pool_.reset();
  framebuffers_.clear();
  render_pass_.reset();
  target_.reset();
}

void FeedbackFullscreenPass::update(const FeedbackFullscreenPassDesc &desc) {
  desc_ = desc;
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

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {writeTarget.width(), writeTarget.height()}},
      .clearValues = desc_.clearValues};

  executor_.beginPass(*framebuffer, *render_pass_, beginDesc);
  executor_.setViewportAndScissor({writeTarget.width(), writeTarget.height()});

  if (pipeline_ && pipeline_->valid()) {
    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    executor_.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    executor_.drawFullscreenTriangle();
  }

  executor_.endPass();
}

auto FeedbackFullscreenPass::addSource(FullscreenPassSource source)
    -> FeedbackFullscreenPass & {
  sources_.push_back(source);
  return *this;
}

auto FeedbackFullscreenPass::setSources(
    std::vector<FullscreenPassSource> sources) -> FeedbackFullscreenPass & {
  sources_ = std::move(sources);
  return *this;
}

auto FeedbackFullscreenPass::target() -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::target() const -> const OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::target(uint32_t frameIndex) -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(frameIndex);
}

auto FeedbackFullscreenPass::target(uint32_t frameIndex) const
    -> const OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' target requested before "
                   "create",
                   name());
  }

  return target_->writeForFrame(frameIndex);
}

auto FeedbackFullscreenPass::historyTarget() -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target requested "
                   "before create",
                   name());
  }

  return target_->readForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::historyTarget() const -> const OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target requested "
                   "before create",
                   name());
  }

  return target_->readForFrame(executor_.frameIndex());
}

auto FeedbackFullscreenPass::historyTarget(uint32_t frameIndex)
    -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target requested "
                   "before create",
                   name());
  }

  return target_->readForFrame(frameIndex);
}

auto FeedbackFullscreenPass::historyTarget(uint32_t frameIndex) const
    -> const OffscreenTarget & {
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

  target_ = std::make_unique<FrameHistoryTarget>(device_, command_pool_);
  target_->update(targetDesc);
}

void FeedbackFullscreenPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  const auto &writeTarget = target_->writeForFrame(0);
  render_pass_->update(pipeline::RenderPassDesc::makeOffscreen(
      writeTarget.color().desc().format,
      writeTarget.depth() ? writeTarget.depth()->desc().format
                          : VK_FORMAT_UNDEFINED));
}

void FeedbackFullscreenPass::createFramebuffers() {
  framebuffers_.clear();
  framebuffers_.resize(target_->targetCount());

  for (uint32_t index = 0; index < framebuffers_.size(); ++index) {
    auto &offscreen = target_->target(index);
    FramebufferDesc framebufferDesc{
        .width = offscreen.width(),
        .height = offscreen.height(),
        .layers = 1,
        .attachments = {offscreen.attachmentViews()}};

    framebuffers_[index] =
        std::make_unique<FramebufferSet>(device_, *render_pass_);
    framebuffers_[index]->update(framebufferDesc);
  }
}

void FeedbackFullscreenPass::createDescriptors() {
  const auto inputs = resolvedInputs();
  if (inputs.empty() && !desc_.historyInput &&
      desc_.descriptorBindings.empty()) {
    return;
  }

  std::vector<pipeline::DescriptorBinding> bindings = desc_.descriptorBindings;
  bindings.reserve(desc_.descriptorBindings.size() + inputs.size() +
                   (desc_.historyInput ? 1U : 0U));

  if (desc_.historyInput) {
    bindings.push_back(pipeline::DescriptorBinding{
        .name = "history",
        .layout = {desc_.historyInput->binding,
                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   desc_.historyInput->stageFlags}});
  }

  for (size_t index = 0; index < inputs.size(); ++index) {
    const auto &input = inputs[index];
    bindings.push_back(pipeline::DescriptorBinding{
        .name = "source" + std::to_string(index),
        .layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   input.stageFlags}});
  }

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(descriptorPoolDesc(inputs));

  descriptor_layout_ = std::make_unique<pipeline::DescriptorSetLayout>(device_);
  validateUniqueDescriptorBindings(name(), bindings);
  descriptor_layout_->update(
      pipeline::DescriptorSetLayoutDesc{.bindings = bindings});

  descriptor_sets_ = std::make_unique<pipeline::DescriptorSets>(device_);
  descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_->pool(),
      .layout = descriptor_layout_->layout(),
      .setCount = executor_.framesInFlight(),
      .writes = createDescriptorWrites(inputs),
  });
}

void FeedbackFullscreenPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;
  pipelineDesc.renderPass = render_pass_->renderPass();

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_ ? descriptor_layout_->layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  if (!pipelineDesc.isValid()) {
    VKR_EXEC_WARN("FeedbackFullscreenPass '{}' has no valid graphics "
                  "pipeline desc",
                  name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' failed to create graphics "
                   "pipeline '{}'",
                   name(), pipelineDesc.name);
  }
}

auto FeedbackFullscreenPass::resolvedInputs() const
    -> std::vector<FullscreenPassInputDesc> {
  if (desc_.inputs.empty()) {
    std::vector<FullscreenPassInputDesc> inputs{};
    inputs.reserve(sources_.size());

    const uint32_t firstBinding =
        nextBindingAfter(desc_.descriptorBindings, desc_.historyInput);
    for (uint32_t index = 0; index < sources_.size(); ++index) {
      inputs.push_back(
          FullscreenPassInputDesc{.binding = firstBinding + index});
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

auto FeedbackFullscreenPass::descriptorPoolDesc(
    const std::vector<FullscreenPassInputDesc> &inputs) const
    -> pipeline::DescriptorPoolDesc {
  return defaultFeedbackDescriptorPoolDesc(
      desc_.descriptorPool, desc_.descriptorBindings,
      desc_.historyInput.has_value(), inputs.size(),
      executor_.framesInFlight());
}

auto FeedbackFullscreenPass::createDescriptorWrites(
    const std::vector<FullscreenPassInputDesc> &inputs)
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  const uint32_t frameCount = executor_.framesInFlight();
  writes.reserve(frameCount);

  for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWriteDesc::forSet(frameIndex));
  }

  appendResourceDescriptorWrites(name(), scene_, desc_.descriptorBindings,
                                 writes, frameCount);

  if (desc_.historyInput) {
    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
      const auto &color = historyTarget(frameIndex).color();
      if (!color.hasSampler()) {
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' history target has no "
                       "sampler",
                       name());
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = imageLayoutForColor(color);
      imageInfo.imageView = color.imageView();
      imageInfo.sampler = color.sampler();

      writes[frameIndex].images.push_back(
          pipeline::DescriptorImageWriteDesc::one(
              desc_.historyInput->binding,
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo));
    }
  }

  for (size_t index = 0; index < sources_.size(); ++index) {
    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
      const auto &color = sources_[index].target(frameIndex).color();
      if (!color.hasSampler()) {
        VKR_EXEC_ERROR("FeedbackFullscreenPass '{}' source {} has no "
                       "sampler",
                       name(), index);
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = imageLayoutForColor(color);
      imageInfo.imageView = color.imageView();
      imageInfo.sampler = color.sampler();

      writes[frameIndex].images.push_back(
          pipeline::DescriptorImageWriteDesc::one(
              inputs[index].binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              imageInfo));
    }
  }

  return writes;
}

} // namespace vkr::exec
