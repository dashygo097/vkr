#include "vkr/exec/render/passes/skybox.hh"
#include "vkr/logger.hh"
#include "vkr/scene/geometry/vbos.hh"

namespace vkr::exec {

SkyboxPass::SkyboxPass(Executor &executor, const core::Device &device,
                       const core::CommandPool &commandPool,
                       scene::Scene &scene)
    : executor_(executor), device_(device), command_pool_(commandPool),
      scene_(scene) {}

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
    VKR_EXEC_ERROR("SkyboxPass '{}' recorded before create", name());
  }

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  executor_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  executor_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    auto mesh = scene_.getMesh(desc_.meshName);
    if (!mesh || !mesh->isValid()) {
      VKR_EXEC_ERROR("SkyboxPass '{}' mesh resource not found: {}", name(),
                       desc_.meshName);
    }

    const auto vertexBuffer = mesh->vertexBufferBase();
    const auto indexBuffer = mesh->indexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      VKR_EXEC_ERROR("SkyboxPass '{}' mesh '{}' has invalid buffers", name(),
                       desc_.meshName);
    }

    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    executor_.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    executor_.drawIndexed(vertexBuffer->get(), indexBuffer->get());
  }

  executor_.endPass();
}

auto SkyboxPass::target() -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("SkyboxPass '{}' target requested before create", name());
  }

  return *target_;
}

auto SkyboxPass::target() const -> const OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("SkyboxPass '{}' target requested before create", name());
  }

  return *target_;
}

void SkyboxPass::createTarget() {
  target_ = std::make_unique<OffscreenTarget>(device_, command_pool_);
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
  FramebufferDesc framebufferDesc{
      .width = target_->width(),
      .height = target_->height(),
      .layers = 1,
      .attachments = {target_->attachmentViews()}};

  framebuffers_ =
      std::make_unique<FramebufferSet>(device_, *render_pass_);
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
      .setCount = executor_.framesInFlight(),
      .writes = createDescriptorWrites(),
  });
}

void SkyboxPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;
  pipelineDesc.renderPass = render_pass_->renderPass();

  if (desc_.configureSkyboxPipelineState) {
    pipelineDesc.vertexInput = scene::VertexSkybox3D::vertexInputDesc();
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
    VKR_EXEC_WARN("SkyboxPass '{}' has no valid graphics pipeline desc",
                    name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("SkyboxPass '{}' failed to create graphics pipeline '{}'",
                     name(), pipelineDesc.name);
  }
}

auto SkyboxPass::descriptorPoolDesc() const -> pipeline::DescriptorPoolDesc {
  auto poolDesc = desc_.descriptorPool;
  const uint32_t frameCount = executor_.framesInFlight();

  if (poolDesc.maxSets == 0) {
    poolDesc.poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frameCount},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frameCount},
    };
    poolDesc.maxSets = frameCount;
  }

  return poolDesc;
}

auto SkyboxPass::createDescriptorWrites() const
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  auto uniformBuffer = scene_.getUniformBuffer(desc_.uniformName);
  if (!uniformBuffer) {
    VKR_EXEC_ERROR("SkyboxPass '{}' uniform resource not found: {}", name(),
                     desc_.uniformName);
  }

  const uint32_t frameCount = executor_.framesInFlight();
  if (uniformBuffer->frameCount() != frameCount) {
    VKR_EXEC_ERROR("SkyboxPass '{}' uniform frame count mismatch: {} vs {}",
                     name(), uniformBuffer->frameCount(), frameCount);
  }

  auto cubemap = scene_.getCubemap(desc_.cubemapName);
  if (!cubemap) {
    VKR_EXEC_ERROR("SkyboxPass '{}' cubemap resource not found: {}", name(),
                     desc_.cubemapName);
  }

  if (!cubemap->valid()) {
    VKR_EXEC_ERROR("SkyboxPass '{}' cubemap has no sampler: {}", name(),
                     desc_.cubemapName);
  }

  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  writes.reserve(frameCount);

  VkDescriptorImageInfo imageInfo = cubemap->descriptorInfo();

  for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
    auto write = pipeline::DescriptorSetWriteDesc::forSet(frameIndex);

    const auto bufferInfo = uniformBuffer->descriptorInfo(frameIndex);

    write.buffers.push_back(pipeline::DescriptorBufferWriteDesc::one(
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfo));
    write.images.push_back(pipeline::DescriptorImageWriteDesc::one(
        1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo));

    writes.push_back(std::move(write));
  }

  return writes;
}

} // namespace vkr::exec
