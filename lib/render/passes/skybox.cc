#include "vkr/render/passes/skybox.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/vbos.hh"

namespace vkr::render {

SkyboxPass::SkyboxPass(Renderer &renderer, const core::Device &device,
                       const core::CommandPool &commandPool,
                       resource::ResourceManager &resourceManager)
    : renderer_(renderer), device_(device), command_pool_(commandPool),
      resource_manager_(resourceManager) {}

SkyboxPass::~SkyboxPass() { destroy(); }

void SkyboxPass::create() {
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void SkyboxPass::destroy() {
  pipeline_.reset();
  descriptor_sets_.reset();
  descriptor_layout_.reset();
  descriptor_pool_.reset();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void SkyboxPass::update(const SkyboxPassDesc &desc) { desc_ = desc; }

void SkyboxPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_RENDER_ERROR("SkyboxPass '{}' recorded before create", name());
  }

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  renderer_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  renderer_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    auto mesh = resource_manager_.getMesh(desc_.meshName);
    if (!mesh || !mesh->isValid()) {
      VKR_RENDER_ERROR("SkyboxPass '{}' mesh resource not found: {}", name(),
                       desc_.meshName);
    }

    auto *vertexBuffer = mesh->vertexBufferBase();
    auto *indexBuffer = mesh->indexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      VKR_RENDER_ERROR("SkyboxPass '{}' mesh '{}' has invalid buffers", name(),
                       desc_.meshName);
    }

    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    renderer_.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    renderer_.drawIndexed(*vertexBuffer, *indexBuffer);
  }

  renderer_.endPass();
}

auto SkyboxPass::target() -> resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("SkyboxPass '{}' target requested before create", name());
  }

  return *target_;
}

auto SkyboxPass::target() const -> const resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("SkyboxPass '{}' target requested before create", name());
  }

  return *target_;
}

void SkyboxPass::createTarget() {
  target_ = std::make_unique<resource::OffscreenTarget>(device_, command_pool_);
  target_->update(desc_.target);
}

void SkyboxPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(pipeline::RenderPassDesc::makeOffscreen(
      target_->color().desc().format, target_->depth()
                                          ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED));
}

void SkyboxPass::createFramebuffers() {
  resource::FramebufferDesc framebufferDesc{
      .width = target_->width(),
      .height = target_->height(),
      .layers = 1,
      .attachments = {target_->attachmentViews()}};

  framebuffers_ =
      std::make_unique<resource::FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void SkyboxPass::createDescriptors() {
  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(descriptorPoolDesc());

  descriptor_layout_ = std::make_unique<pipeline::DescriptorSetLayout>(device_);
  descriptor_layout_->update(pipeline::DescriptorSetLayoutDesc{
      .bindings = {
          {.name = desc_.uniformName,
           .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                      VK_SHADER_STAGE_VERTEX_BIT}},
          {.name = desc_.cubemapName,
           .layout = {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                      VK_SHADER_STAGE_FRAGMENT_BIT}},
      }});

  descriptor_sets_ = std::make_unique<pipeline::DescriptorSets>(device_);
  descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_->pool(),
      .layout = descriptor_layout_->layout(),
      .setCount = core::MAX_FRAMES_IN_FLIGHT,
      .writes = createDescriptorWrites(),
  });
}

void SkyboxPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;
  pipelineDesc.renderPass = render_pass_->renderPass();

  if (desc_.configureSkyboxPipelineState) {
    pipelineDesc.vertexInput = resource::VertexSkybox3D::vertexInputDesc();
    pipelineDesc.rasterization = pipeline::GraphicsRasterizationDesc::noCull();
    pipelineDesc.depthStencil = pipeline::GraphicsDepthStencilDesc::readOnly();
  }

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_ ? descriptor_layout_->layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  if (!pipelineDesc.isValid()) {
    VKR_RENDER_WARN("SkyboxPass '{}' has no valid graphics pipeline desc",
                    name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_RENDER_ERROR("SkyboxPass '{}' failed to create graphics pipeline '{}'",
                     name(), pipelineDesc.name);
  }
}

auto SkyboxPass::descriptorPoolDesc() const -> pipeline::DescriptorPoolDesc {
  auto poolDesc = desc_.descriptorPool;

  if (poolDesc.maxSets == 0) {
    poolDesc.poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, core::MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         core::MAX_FRAMES_IN_FLIGHT},
    };
    poolDesc.maxSets = core::MAX_FRAMES_IN_FLIGHT;
  }

  return poolDesc;
}

auto SkyboxPass::createDescriptorWrites() const
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  auto uniformBuffer = resource_manager_.getUniformBuffer(desc_.uniformName);
  if (!uniformBuffer) {
    VKR_RENDER_ERROR("SkyboxPass '{}' uniform resource not found: {}", name(),
                     desc_.uniformName);
  }

  const auto &buffers = uniformBuffer->buffers();
  if (buffers.size() != core::MAX_FRAMES_IN_FLIGHT) {
    VKR_RENDER_ERROR("SkyboxPass '{}' uniform frame count mismatch: {} vs {}",
                     name(), buffers.size(), core::MAX_FRAMES_IN_FLIGHT);
  }

  auto cubemap = resource_manager_.getTexture(desc_.cubemapName);
  if (!cubemap) {
    VKR_RENDER_ERROR("SkyboxPass '{}' cubemap resource not found: {}", name(),
                     desc_.cubemapName);
  }

  if (!cubemap->hasSampler()) {
    VKR_RENDER_ERROR("SkyboxPass '{}' cubemap has no sampler: {}", name(),
                     desc_.cubemapName);
  }

  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  writes.reserve(core::MAX_FRAMES_IN_FLIGHT);

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = cubemap->layout();
  imageInfo.imageView = cubemap->imageView();
  imageInfo.sampler = cubemap->sampler();

  for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
       ++frameIndex) {
    auto write = pipeline::DescriptorSetWriteDesc::forSet(frameIndex);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[frameIndex];
    bufferInfo.offset = 0;
    bufferInfo.range = uniformBuffer->bufferSize();

    write.buffers.push_back(pipeline::DescriptorBufferWriteDesc::one(
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfo));
    write.images.push_back(pipeline::DescriptorImageWriteDesc::one(
        1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo));

    writes.push_back(std::move(write));
  }

  return writes;
}

} // namespace vkr::render
