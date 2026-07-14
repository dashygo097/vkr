#include "vkr/render/passes/fullscreen_pass.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/logger.hh"
#include "vkr/render/passes/raster.hh"
#include "vkr/render/passes/skybox.hh"

namespace vkr::render {

FullscreenPassSource::FullscreenPassSource(RasterPass &source)
    : type(Type::Raster), raster(&source) {}

FullscreenPassSource::FullscreenPassSource(SkyboxPass &source)
    : type(Type::Skybox), skybox(&source) {}

FullscreenPassSource::FullscreenPassSource(FullscreenPass &source)
    : type(Type::Fullscreen), fullscreen(&source) {}

auto FullscreenPassSource::target() -> resource::OffscreenTarget & {
  switch (type) {
  case Type::Raster:
    if (!raster) {
      VKR_RENDER_ERROR("FullscreenPassSource has null raster source");
    }
    return raster->target();
  case Type::Skybox:
    if (!skybox) {
      VKR_RENDER_ERROR("FullscreenPassSource has null skybox source");
    }
    return skybox->target();
  case Type::Fullscreen:
    if (!fullscreen) {
      VKR_RENDER_ERROR("FullscreenPassSource has null fullscreen source");
    }
    return fullscreen->target();
  }

  VKR_RENDER_ERROR("FullscreenPassSource has unknown source type");
}

auto FullscreenPassSource::target() const
    -> const resource::OffscreenTarget & {
  switch (type) {
  case Type::Raster:
    if (!raster) {
      VKR_RENDER_ERROR("FullscreenPassSource has null raster source");
    }
    return raster->target();
  case Type::Skybox:
    if (!skybox) {
      VKR_RENDER_ERROR("FullscreenPassSource has null skybox source");
    }
    return skybox->target();
  case Type::Fullscreen:
    if (!fullscreen) {
      VKR_RENDER_ERROR("FullscreenPassSource has null fullscreen source");
    }
    return fullscreen->target();
  }

  VKR_RENDER_ERROR("FullscreenPassSource has unknown source type");
}

FullscreenPass::FullscreenPass(Renderer &renderer, const core::Device &device,
                               const core::CommandPool &commandPool,
                               std::vector<FullscreenPassSource> sources)
    : renderer_(renderer), device_(device), command_pool_(commandPool),
      sources_(std::move(sources)) {}

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
    VKR_RENDER_ERROR("FullscreenPass '{}' recorded before create", name());
  }

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  renderer_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  renderer_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    renderer_.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    renderer_.drawFullscreenTriangle();
  }

  renderer_.endPass();
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

auto FullscreenPass::target() -> resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("FullscreenPass '{}' target requested before create",
                     name());
  }

  return *target_;
}

auto FullscreenPass::target() const -> const resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("FullscreenPass '{}' target requested before create",
                     name());
  }

  return *target_;
}

void FullscreenPass::createTarget() {
  target_ = std::make_unique<resource::OffscreenTarget>(device_, command_pool_);
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
  resource::FramebufferDesc framebufferDesc{
      .width = target_->width(),
      .height = target_->height(),
      .layers = 1,
      .attachments = {target_->attachmentViews()}};

  framebuffers_ =
      std::make_unique<resource::FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void FullscreenPass::createDescriptors() {
  const auto inputs = resolvedInputs();
  if (inputs.empty()) {
    return;
  }

  std::vector<pipeline::DescriptorBinding> bindings{};
  bindings.reserve(inputs.size());

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
    VKR_RENDER_WARN("FullscreenPass '{}' has no valid graphics pipeline desc",
                    name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_RENDER_ERROR(
        "FullscreenPass '{}' failed to create graphics pipeline '{}'", name(),
        pipelineDesc.name);
  }
}

auto FullscreenPass::resolvedInputs() const
    -> std::vector<FullscreenPassInputDesc> {
  if (desc_.inputs.empty()) {
    std::vector<FullscreenPassInputDesc> inputs{};
    inputs.reserve(sources_.size());

    for (uint32_t index = 0; index < sources_.size(); ++index) {
      inputs.push_back(FullscreenPassInputDesc{.binding = index});
    }

    return inputs;
  }

  if (desc_.inputs.size() != sources_.size()) {
    VKR_RENDER_ERROR(
        "FullscreenPass '{}' input count mismatch: desc={} sources={}", name(),
        desc_.inputs.size(), sources_.size());
  }

  return desc_.inputs;
}

auto FullscreenPass::descriptorPoolDesc(
    const std::vector<FullscreenPassInputDesc> &inputs) const
    -> pipeline::DescriptorPoolDesc {
  auto poolDesc = desc_.descriptorPool;

  if (poolDesc.maxSets == 0) {
    poolDesc.poolSizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         static_cast<uint32_t>(core::MAX_FRAMES_IN_FLIGHT * inputs.size())}};
    poolDesc.maxSets = core::MAX_FRAMES_IN_FLIGHT;
  }

  return poolDesc;
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

  for (size_t index = 0; index < sources_.size(); ++index) {
    const auto &color = sources_[index].target().color();
    if (!color.hasSampler()) {
      VKR_RENDER_ERROR("FullscreenPass '{}' source {} has no sampler", name(),
                       index);
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = color.desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
                                ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                : color.desc().finalLayout;
    imageInfo.imageView = color.imageView();
    imageInfo.sampler = color.sampler();

    for (auto &write : writes) {
      write.images.push_back(pipeline::DescriptorImageWriteDesc::one(
          inputs[index].binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          imageInfo));
    }
  }

  return writes;
}

} // namespace vkr::render
