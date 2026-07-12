#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace vkr {

void VulkanApplication::initVulkan() {
  Logger::init();
  onConfigure();
  loadSnapshot();

  if (!ctx.isValid()) {
    VKR_CORE_ERROR("invalid Vulkan context config");
  }

  // asset
  assetSystem = std::make_unique<util::AssetSystem>(ctx.asset);

  // window
  window = std::make_unique<core::Window>(ctx.window);
  glfwSetWindowUserPointer(window->glfwWindow(), this);

  // instance
  instance = std::make_unique<core::Instance>(ctx.instance);

  // surface
  surface = std::make_unique<core::Surface>(*instance, *window);

  // device
  device = std::make_unique<core::Device>(*instance, *surface, ctx.device);

  // swapchain
  swapchain = std::make_unique<core::Swapchain>(*window, *surface, *device,
                                                ctx.swapchain);

  // command pool
  commandPool = std::make_unique<core::CommandPool>(*device);

  // command buffers
  commandBuffers =
      std::make_unique<core::CommandBuffers>(*device, *commandPool);

  // sync objects
  syncObjects = std::make_unique<core::SyncObjects>(*device, *swapchain);

  // swapchain target
  swapchainTarget = std::make_unique<resource::SwapchainTarget>(
      *device, *commandPool, *swapchain);

  resource::SwapchainTargetDesc swapchainTargetDesc{
      .depth = resource::DepthAttachmentDesc{.format = VK_FORMAT_D32_SFLOAT}};
  swapchainTarget->update(swapchainTargetDesc);

  // resource manager
  resourceManager =
      std::make_unique<resource::ResourceManager>(*device, *commandPool);

  // render pass: swapchain
  swapchainRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  swapchainRenderPass->update(pipeline::RenderPassDesc::makeSwapchain(
      swapchainTarget->format(), swapchainTarget->depth()->desc().format));

  // framebuffer set: swapchain
  resource::FramebufferDesc swapchainFbDesc{
      .width = swapchainTarget->width(),
      .height = swapchainTarget->height(),
      .layers = 1,
      .attachments = swapchainTarget->attachmentViews()};

  swapchainFramebufferSet =
      std::make_unique<resource::FramebufferSet>(*device, *swapchainRenderPass);
  swapchainFramebufferSet->update(swapchainFbDesc);

  // offscreen target
  resource::OffscreenTargetDesc offscreenDesc{
      .color = {.width = swapchain->width(),
                .height = swapchain->height(),
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                         VK_IMAGE_USAGE_SAMPLED_BIT,
                .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .createSampler = true},
      .depth = resource::DepthAttachmentDesc{.width = swapchain->width(),
                                             .height = swapchain->height(),
                                             .format = VK_FORMAT_D32_SFLOAT}};

  offscreenTarget =
      std::make_unique<resource::OffscreenTarget>(*device, *commandPool);
  offscreenTarget->update(offscreenDesc);

  // render pass: offscreen
  offscreenRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  offscreenRenderPass->update(pipeline::RenderPassDesc::makeOffscreen(
      offscreenTarget->color().desc().format,
      offscreenTarget->depth()->desc().format));

  // framebuffer set: offscreen
  resource::FramebufferDesc offscreenFbDesc{
      .width = offscreenTarget->width(),
      .height = offscreenTarget->height(),
      .layers = 1,
      .attachments = {offscreenTarget->attachmentViews()}};

  offscreenFramebufferSet =
      std::make_unique<resource::FramebufferSet>(*device, *offscreenRenderPass);
  offscreenFramebufferSet->update(offscreenFbDesc);

  // user resources
  createResources();

  // descriptor pool
  pipeline::DescriptorPoolDesc descPoolDesc{
      .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
      .maxSets = core::MAX_FRAMES_IN_FLIGHT + 2};

  descriptorPool = std::make_unique<pipeline::DescriptorPool>(*device);
  descriptorPool->update(descPoolDesc);

  // descriptor layout
  descriptorSetLayout =
      std::make_unique<pipeline::DescriptorSetLayout>(*device);
  descriptorSetLayout->update(pipeline::DescriptorSetLayoutDesc{
      .bindings = createDescriptorBindings(),
  });

  // pipeline library
  pipelineLibrary = std::make_unique<render::PipelineLibrary>(*device);
  createPipelines();

  if (pipelineLibrary->empty()) {
    VKR_CORE_WARN("No graphics pipelines were created");
  }

  // descriptor sets
  descriptorSets = std::make_unique<pipeline::DescriptorSets>(*device);
  descriptorSets->update(pipeline::DescriptorSetsDesc{
      .pool = descriptorPool->pool(),
      .layout = descriptorSetLayout->layout(),
      .setCount = core::MAX_FRAMES_IN_FLIGHT,
      .writes = createDescriptorSetWrites(),
  });

  // timer
  timer = std::make_unique<util::Timer>();

  // ui
  ui = std::make_unique<ui::UI>(
      *window, *instance, *surface, *device, *commandPool, *resourceManager,
      *offscreenTarget, *swapchainRenderPass, *descriptorPool, *pipelineLibrary,
      *timer, ctx.theme);

  // renderer
  renderer = std::make_unique<render::Renderer>(
      *device, *swapchain, *commandPool, *syncObjects, *resourceManager, *ui);

  // camera
  camera = std::make_unique<scene::Camera>(*window, *timer, ctx.camera);
}

void VulkanApplication::mainLoop() {
  timer->start();
  bool isLastTabKeyPressed = false;

  while (!window->shouldClose() && !ui->shouldClose()) {
    timer->beginFrame();

    window->pollEvents();

    bool isNowTabKeyPressed =
        glfwGetKey(window->glfwWindow(), GLFW_KEY_TAB) == GLFW_PRESS;

    if (!camera->isLocked()) {
      camera->track();
    }

    if (isNowTabKeyPressed && !isLastTabKeyPressed) {
      ui->switchLayoutMode();
    }

    bool shouldLockCamera = false;
    if (ui->layoutMode() == ui::LayoutMode::Standard) {
      shouldLockCamera = !ui->viewportInfo().isHovered;
    }

    camera->lock(shouldLockCamera);

    timer->update();
    drawFrame();

    isLastTabKeyPressed = isNowTabKeyPressed;
    timer->endFrame();
  }

  device->waitIdle();
}

void VulkanApplication::drawFrame() {
  if (!renderer->beginFrame()) {
    return;
  }

  render::RenderPassBeginDesc offscreenDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {offscreenTarget->width(),
                                offscreenTarget->height()}},
      .clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                      VkClearValue{.depthStencil = {1.0f, 0}}}};

  renderer->beginPass(*offscreenFramebufferSet, *offscreenRenderPass,
                      offscreenDesc);

  renderer->setViewportAndScissor(
      {offscreenTarget->width(), offscreenTarget->height()});

  if (!pipelineLibrary->empty()) {
    auto &pipeline = pipelineLibrary->first();

    renderer->bindPipeline(pipeline.pipeline(), pipeline.layout(),
                           descriptorSets->sets());
    renderer->drawGeometry();
  }

  renderer->endPass();

  render::RenderPassBeginDesc swapchainDesc{
      .framebufferIndex = renderer->imageIndex(),
      .renderArea = {.offset = {0, 0},
                     .extent = {swapchainTarget->width(),
                                swapchainTarget->height()}},
      .clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                      VkClearValue{.depthStencil = {1.0f, 0}}}};

  renderer->beginPass(*swapchainFramebufferSet, *swapchainRenderPass,
                      swapchainDesc);

  renderer->setViewportAndScissor(
      {swapchainTarget->width(), swapchainTarget->height()});

  renderer->drawUI();

  renderer->endPass();

  onDrawFrame(renderer->frameIndex());

  renderer->endFrame();
}

auto VulkanApplication::createDescriptorSetWrites()
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  const auto frameCount = core::MAX_FRAMES_IN_FLIGHT;
  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  writes.reserve(frameCount);

  for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWriteDesc::forSet(frameIndex));
  }

  for (const auto &binding : descriptorSetLayout->bindings()) {
    if (binding.name.empty()) {
      VKR_PIPE_ERROR("Descriptor binding {} has empty resource name",
                     binding.layout.binding);
    }

    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = resourceManager->getUniformBuffer(binding.name);

      if (!uniformBuffer) {
        VKR_PIPE_ERROR("Uniform buffer resource not found: {}", binding.name);
      }

      const auto &buffers = uniformBuffer->buffers();
      if (buffers.size() != frameCount) {
        VKR_PIPE_ERROR("Uniform buffer '{}' frame count mismatch: {} vs {}",
                       binding.name, buffers.size(), frameCount);
      }

      for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
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
      auto texture = resourceManager->getTexture(binding.name);

      if (!texture) {
        VKR_PIPE_ERROR("Texture resource not found: {}", binding.name);
      }

      if (!texture->hasSampler()) {
        VKR_PIPE_ERROR("Texture sampler not found: {}", binding.name);
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
      VKR_PIPE_ERROR("No default descriptor write builder for resource '{}' "
                     "at binding {} with descriptor type {}",
                     binding.name, binding.layout.binding,
                     static_cast<int>(binding.layout.descriptorType));
      break;
    }
  }

  return writes;
}

} // namespace vkr
