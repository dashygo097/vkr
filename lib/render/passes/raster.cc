#include "vkr/render/passes/raster.hh"
#include "vkr/logger.hh"
#include "vkr/render/renderer.hh"

namespace vkr::render {

RasterPass::RasterPass(const core::Device &device,
                       const core::CommandPool &commandPool,
                       resource::ResourceManager &resourceManager,
                       RasterPassDesc desc)
    : device_(device), command_pool_(commandPool),
      resource_manager_(resourceManager) {
  update(desc);
}

RasterPass::~RasterPass() { destroy(); }

void RasterPass::create() {
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void RasterPass::destroy() {
  pipeline_.reset();
  descriptor_sets_.reset();
  descriptor_layout_.reset();
  descriptor_pool_.reset();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void RasterPass::update(const RenderGraphPassDesc &desc) {
  desc_.graph = desc;
  setDesc(desc_.graph);
}

void RasterPass::update(const RasterPassDesc &desc) {
  desc_ = desc;
  setDesc(desc_.graph);
}

void RasterPass::record(Renderer &renderer) {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_RENDER_ERROR("RasterPass '{}' recorded before create", name());
  }

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  renderer.beginPass(*framebuffers_, *render_pass_, beginDesc);
  renderer.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    renderer.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    renderer.drawGeometry();
  }

  renderer.endPass();
}

auto RasterPass::target() -> resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

auto RasterPass::target() const -> const resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

void RasterPass::createTarget() {
  target_ = std::make_unique<resource::OffscreenTarget>(device_, command_pool_);
  target_->update(desc_.target);
}

void RasterPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(pipeline::RenderPassDesc::makeOffscreen(
      target_->color().desc().format, target_->depth()
                                          ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED));
}

void RasterPass::createFramebuffers() {
  resource::FramebufferDesc framebufferDesc{
      .width = target_->width(),
      .height = target_->height(),
      .layers = 1,
      .attachments = {target_->attachmentViews()}};

  framebuffers_ =
      std::make_unique<resource::FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void RasterPass::createDescriptors() {
  if (desc_.descriptorBindings.empty()) {
    return;
  }

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(desc_.descriptorPool);

  descriptor_layout_ = std::make_unique<pipeline::DescriptorSetLayout>(device_);
  descriptor_layout_->update(
      pipeline::DescriptorSetLayoutDesc{.bindings = desc_.descriptorBindings});

  descriptor_sets_ = std::make_unique<pipeline::DescriptorSets>(device_);
  descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_->pool(),
      .layout = descriptor_layout_->layout(),
      .setCount = core::MAX_FRAMES_IN_FLIGHT,
      .writes = createDescriptorWrites(),
  });
}

void RasterPass::createPipeline() {
  if (!desc_.pipeline) {
    VKR_RENDER_WARN("RasterPass '{}' has no graphics pipeline factory", name());
    return;
  }

  RasterPipelineBuildInfo buildInfo{
      .renderPass = render_pass_->renderPass(),
      .descriptorSetLayout =
          descriptor_layout_ ? descriptor_layout_->layout() : VK_NULL_HANDLE,
      .extent = {target_->width(), target_->height()}};

  auto pipelineDesc = desc_.pipeline(buildInfo);
  pipelineDesc.renderPass = buildInfo.renderPass;

  if (buildInfo.descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {buildInfo.descriptorSetLayout};
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_RENDER_ERROR("RasterPass '{}' failed to create graphics pipeline '{}'",
                     name(), pipelineDesc.name);
  }
}

auto RasterPass::createDescriptorWrites() const
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  writes.reserve(core::MAX_FRAMES_IN_FLIGHT);

  for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
       ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWriteDesc::forSet(frameIndex));
  }

  for (const auto &binding : desc_.descriptorBindings) {
    if (binding.name.empty()) {
      VKR_RENDER_ERROR("Descriptor binding {} has empty resource name",
                       binding.layout.binding);
    }

    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = resource_manager_.getUniformBuffer(binding.name);

      if (!uniformBuffer) {
        VKR_RENDER_ERROR("Uniform buffer resource not found: {}", binding.name);
      }

      const auto &buffers = uniformBuffer->buffers();
      if (buffers.size() != core::MAX_FRAMES_IN_FLIGHT) {
        VKR_RENDER_ERROR("Uniform buffer '{}' frame count mismatch: {} vs {}",
                         binding.name, buffers.size(),
                         core::MAX_FRAMES_IN_FLIGHT);
      }

      for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
           ++frameIndex) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers[frameIndex];
        bufferInfo.offset = 0;
        bufferInfo.range = uniformBuffer->bufferSize();

        writes[frameIndex].buffers.push_back(
            pipeline::DescriptorBufferWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                bufferInfo));
      }
      break;
    }

    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      auto texture = resource_manager_.getTexture(binding.name);

      if (!texture) {
        VKR_RENDER_ERROR("Texture resource not found: {}", binding.name);
      }

      if (!texture->hasSampler()) {
        VKR_RENDER_ERROR("Texture sampler not found: {}", binding.name);
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
      VKR_RENDER_ERROR("RasterPass '{}' cannot create descriptor writes for "
                       "resource '{}' with descriptor type {}",
                       name(), binding.name,
                       static_cast<int>(binding.layout.descriptorType));
    }
  }

  return writes;
}

} // namespace vkr::render
