#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/logger.hh"
#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/exec/render/passes/raster.hh"
#include "vkr/exec/render/passes/skybox.hh"
#include <algorithm>

namespace vkr::exec {
namespace {

auto defaultDescriptorPoolDesc(
    pipeline::DescriptorPoolDesc poolDesc,
    const std::vector<pipeline::DescriptorBinding> &resourceBindings,
    size_t sourceCount) -> pipeline::DescriptorPoolDesc {
  if (poolDesc.maxSets != 0) {
    return poolDesc;
  }

  uint32_t uniformCount = 0;
  uint32_t imageCount = static_cast<uint32_t>(sourceCount);

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
         static_cast<uint32_t>(core::MAX_FRAMES_IN_FLIGHT * uniformCount)});
  }

  if (imageCount > 0) {
    poolDesc.poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         static_cast<uint32_t>(core::MAX_FRAMES_IN_FLIGHT * imageCount)});
  }

  poolDesc.maxSets = core::MAX_FRAMES_IN_FLIGHT;
  return poolDesc;
}

auto nextBindingAfter(
    const std::vector<pipeline::DescriptorBinding> &resourceBindings)
    -> uint32_t {
  uint32_t nextBinding = 0;

  for (const auto &binding : resourceBindings) {
    const uint32_t descriptorCount = binding.layout.descriptorCount == 0
                                         ? 1U
                                         : binding.layout.descriptorCount;
    nextBinding =
        std::max(nextBinding, binding.layout.binding + descriptorCount);
  }

  return nextBinding;
}

void appendResourceDescriptorWrites(
    std::string_view passName, const scene::Scene *scene,
    const std::vector<pipeline::DescriptorBinding> &bindings,
    std::vector<pipeline::DescriptorSetWriteDesc> &writes) {
  if (bindings.empty()) {
    return;
  }

  if (scene == nullptr) {
    VKR_EXEC_ERROR("FullscreenPass '{}' has descriptor resource bindings "
                     "but no Scene",
                     std::string(passName));
  }

  for (const auto &binding : bindings) {
    if (binding.name.empty()) {
      VKR_EXEC_ERROR("FullscreenPass '{}' descriptor binding {} has empty "
                       "resource name",
                       std::string(passName), binding.layout.binding);
    }

    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = scene->getUniformBuffer(binding.name);
      if (!uniformBuffer) {
        VKR_EXEC_ERROR("FullscreenPass '{}' uniform buffer resource not "
                         "found: {}",
                         std::string(passName), binding.name);
      }

      if (uniformBuffer->frameCount() != core::MAX_FRAMES_IN_FLIGHT) {
        VKR_EXEC_ERROR("FullscreenPass '{}' uniform buffer '{}' frame count "
                         "mismatch: {} vs {}",
                         std::string(passName), binding.name,
                         uniformBuffer->frameCount(),
                         core::MAX_FRAMES_IN_FLIGHT);
      }

      for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
           ++frameIndex) {
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
        VKR_EXEC_ERROR("FullscreenPass '{}' texture resource not found: {}",
                         std::string(passName), binding.name);
      }

      if (!texture->hasSampler()) {
        VKR_EXEC_ERROR("FullscreenPass '{}' texture sampler not found: {}",
                         std::string(passName), binding.name);
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = texture->layout();
      imageInfo.imageView = texture->imageView();
      imageInfo.sampler = texture->sampler();

      for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
           ++frameIndex) {
        writes[frameIndex].images.push_back(
            pipeline::DescriptorImageWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                imageInfo));
      }
      break;
    }

    default:
      VKR_EXEC_ERROR("FullscreenPass '{}' cannot create descriptor writes "
                       "for resource '{}' with descriptor type {}",
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
        VKR_EXEC_ERROR("FullscreenPass '{}' has duplicate descriptor binding "
                         "{} for '{}' and '{}'",
                         std::string(passName), bindings[i].layout.binding,
                         bindings[i].name, bindings[j].name);
      }
    }
  }
}

} // namespace

FullscreenPassSource::FullscreenPassSource(RasterPass &source)
    : source_(std::ref(source)) {}

FullscreenPassSource::FullscreenPassSource(SkyboxPass &source)
    : source_(std::ref(source)) {}

FullscreenPassSource::FullscreenPassSource(FullscreenPass &source)
    : source_(std::ref(source)) {}

FullscreenPassSource::FullscreenPassSource(FeedbackFullscreenPass &source)
    : source_(std::ref(source)) {}

auto FullscreenPassSource::target() -> OffscreenTarget & {
  return target(0);
}

auto FullscreenPassSource::target() const -> const OffscreenTarget & {
  return target(0);
}

auto FullscreenPassSource::target(uint32_t frameIndex)
    -> OffscreenTarget & {
  return std::visit(
      [frameIndex](auto source) -> OffscreenTarget & {
        return source.get().target(frameIndex);
      },
      source_);
}

auto FullscreenPassSource::target(uint32_t frameIndex) const
    -> const OffscreenTarget & {
  return std::visit(
      [frameIndex](auto source) -> const OffscreenTarget & {
        return source.get().target(frameIndex);
      },
      source_);
}

FullscreenPass::FullscreenPass(Executor &executor, const core::Device &device,
                               const core::CommandPool &commandPool,
                               std::vector<FullscreenPassSource> sources)
    : executor_(executor), device_(device), command_pool_(commandPool),
      sources_(std::move(sources)) {}

FullscreenPass::FullscreenPass(Executor &executor, const core::Device &device,
                               const core::CommandPool &commandPool,
                               scene::Scene &scene,
                               std::vector<FullscreenPassSource> sources)
    : executor_(executor), device_(device), command_pool_(commandPool),
      scene_(&scene), sources_(std::move(sources)) {}

FullscreenPass::~FullscreenPass() { destroy(); }

void FullscreenPass::create() {
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void FullscreenPass::destroy() {
  pipeline_.reset();
  descriptor_sets_.reset();
  descriptor_layout_.reset();
  descriptor_pool_.reset();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void FullscreenPass::update(const FullscreenPassDesc &desc) { desc_ = desc; }

void FullscreenPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' recorded before create", name());
  }

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  executor_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  executor_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    executor_.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    executor_.drawFullscreenTriangle();
  }

  executor_.endPass();
}

auto FullscreenPass::addSource(FullscreenPassSource source)
    -> FullscreenPass & {
  sources_.push_back(source);
  return *this;
}

auto FullscreenPass::setSources(std::vector<FullscreenPassSource> sources)
    -> FullscreenPass & {
  sources_ = std::move(sources);
  return *this;
}

auto FullscreenPass::target() -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' target requested before create",
                     name());
  }

  return *target_;
}

auto FullscreenPass::target() const -> const OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' target requested before create",
                     name());
  }

  return *target_;
}

void FullscreenPass::createTarget() {
  target_ = std::make_unique<OffscreenTarget>(device_, command_pool_);
  target_->update(desc_.target);
}

void FullscreenPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(pipeline::RenderPassDesc::makeOffscreen(
      target_->color().desc().format, target_->depth()
                                          ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED));
}

void FullscreenPass::createFramebuffers() {
  FramebufferDesc framebufferDesc{
      .width = target_->width(),
      .height = target_->height(),
      .layers = 1,
      .attachments = {target_->attachmentViews()}};

  framebuffers_ =
      std::make_unique<FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void FullscreenPass::createDescriptors() {
  const auto inputs = resolvedInputs();
  if (inputs.empty() && desc_.descriptorBindings.empty()) {
    return;
  }

  std::vector<pipeline::DescriptorBinding> bindings = desc_.descriptorBindings;
  bindings.reserve(desc_.descriptorBindings.size() + inputs.size());

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
      .setCount = core::MAX_FRAMES_IN_FLIGHT,
      .writes = createDescriptorWrites(inputs),
  });
}

void FullscreenPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;
  pipelineDesc.renderPass = render_pass_->renderPass();

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_ ? descriptor_layout_->layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  if (!pipelineDesc.isValid()) {
    VKR_EXEC_WARN("FullscreenPass '{}' has no valid graphics pipeline desc",
                    name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR(
        "FullscreenPass '{}' failed to create graphics pipeline '{}'", name(),
        pipelineDesc.name);
  }
}

auto FullscreenPass::resolvedInputs() const
    -> std::vector<FullscreenPassInputDesc> {
  if (desc_.inputs.empty()) {
    std::vector<FullscreenPassInputDesc> inputs{};
    inputs.reserve(sources_.size());

    const uint32_t firstBinding = nextBindingAfter(desc_.descriptorBindings);
    for (uint32_t index = 0; index < sources_.size(); ++index) {
      inputs.push_back(
          FullscreenPassInputDesc{.binding = firstBinding + index});
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

auto FullscreenPass::descriptorPoolDesc(
    const std::vector<FullscreenPassInputDesc> &inputs) const
    -> pipeline::DescriptorPoolDesc {
  return defaultDescriptorPoolDesc(desc_.descriptorPool,
                                   desc_.descriptorBindings, inputs.size());
}

auto FullscreenPass::createDescriptorWrites(
    const std::vector<FullscreenPassInputDesc> &inputs)
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  writes.reserve(core::MAX_FRAMES_IN_FLIGHT);

  for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
       ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWriteDesc::forSet(frameIndex));
  }

  appendResourceDescriptorWrites(name(), scene_,
                                 desc_.descriptorBindings, writes);

  for (size_t index = 0; index < sources_.size(); ++index) {
    for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
         ++frameIndex) {
      const auto &color = sources_[index].target(frameIndex).color();
      if (!color.hasSampler()) {
        VKR_EXEC_ERROR("FullscreenPass '{}' source {} has no sampler", name(),
                         index);
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout =
          color.desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
              ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
              : color.desc().finalLayout;
      imageInfo.imageView = color.imageView();
      imageInfo.sampler = color.sampler();

      auto &write = writes[frameIndex];
      write.images.push_back(pipeline::DescriptorImageWriteDesc::one(
          inputs[index].binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          imageInfo));
    }
  }

  return writes;
}

} // namespace vkr::exec
