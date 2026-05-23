#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace vkr {

void VulkanApplication::initVulkan() {
  Logger::init();
  onConfigure();

  // window
  window = std::make_unique<core::Window>(ctx.title, ctx.width, ctx.height);
  glfwSetWindowUserPointer(window->glfwWindow(), this);
  glfwSetFramebufferSizeCallback(window->glfwWindow(),
                                 framebufferResizeCallback);

  // instance
  instance = std::make_unique<core::Instance>(
      ctx.appName, ctx.appVersion, ctx.preExtensions, ctx.validationLayers);

  // surface
  surface = std::make_unique<core::Surface>(*instance, *window);

  // device
  device = std::make_unique<core::Device>(
      *instance, *surface, ctx.deviceExtensions, ctx.validationLayers);

  // swapchain
  swapchain = std::make_unique<core::Swapchain>(*window, *device, *surface);

  // command pool
  commandPool = std::make_unique<core::CommandPool>(*device, *surface);

  // command buffers
  commandBuffers =
      std::make_unique<core::CommandBuffers>(*device, *commandPool);

  // sync objects
  syncObjects = std::make_unique<core::SyncObjects>(*device, *swapchain);

  // render pass (swapchain)
  pipeline::RenderPassDesc swapchainRenderPassDesc =
      pipeline::makeSwapchainRenderPassDesc(*device, *swapchain);
  swapchainRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  swapchainRenderPass->update(swapchainRenderPassDesc);

  // resource manager
  depthResources = std::make_unique<resource::DepthResources>(
      *device, *swapchain, *commandPool);
  resourceManager = std::make_unique<resource::ResourceManager>(
      *device, *swapchain, *commandPool);

  resource::FramebufferDesc swapchainfbDesc;
  swapchainfbDesc.extent = swapchain->extent2D();
  swapchainfbDesc.layers = 1;
  swapchainfbDesc.attachments.reserve(swapchain->images().size());
  for (auto imageView : swapchain->imageViews()) {
    swapchainfbDesc.attachments.push_back(
        {imageView, depthResources->imageView()});
  }

  resourceManager->createFramebufferSet("swapchain", *swapchainRenderPass,
                                        swapchainfbDesc);

  pipeline::RenderPassDesc offscreenRenderPassDesc =
      pipeline::makeOffscreenRenderPassDesc(VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_FORMAT_D32_SFLOAT);
  offscreenRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  offscreenRenderPass->update(offscreenRenderPassDesc);
  offscreenTarget = std::make_unique<resource::OffscreenTarget>(
      *device, *commandPool, *offscreenRenderPass);
  offscreenTarget->resize(swapchain->extent2D());

  vkr::resource::FramebufferDesc offscreenTargetfbDesc{};
  offscreenTargetfbDesc.extent = offscreenTarget->extent2D();
  offscreenTargetfbDesc.layers = 1;
  offscreenTargetfbDesc.attachments = {
      {offscreenTarget->colorView(), offscreenTarget->depthView()},
  };

  resourceManager->createFramebufferSet("offscreen", *offscreenRenderPass,
                                        offscreenTargetfbDesc);

  createUniforms();

  // descriptor pool
  uint32_t maxSets = core::MAX_FRAMES_IN_FLIGHT + 2;
  pipeline::DescriptorPoolSizes poolSizes =
      pipeline::DescriptorManager::calculatePoolSizes(maxSets);
  descriptorPool =
      std::make_unique<pipeline::DescriptorPool>(*device, maxSets, poolSizes);

  // descriptor layout
  descriptorManager = std::make_unique<pipeline::DescriptorManager>(*device);
  std::vector<pipeline::DescriptorBinding> bindings =
      createDescriptorBindings();
  descriptorSetLayout = descriptorManager->createLayout(bindings);

  graphicsPipeline = std::make_unique<pipeline::GraphicsPipeline>(
      *instance, *device, *resourceManager, *swapchainRenderPass,
      *descriptorSetLayout, ctx.pipelineMode);
  graphicsPipeline->buildOffscreen(offscreenRenderPass->renderPass());

  // descriptor sets
  descriptorSets =
      descriptorManager->allocate(*descriptorSetLayout, *descriptorPool);
  bindDescriptorSets();

  // timer
  timer = std::make_unique<Timer>();

  // ui
  ui = std::make_unique<ui::UI>(
      *window, *instance, *surface, *device, *commandPool, *resourceManager,
      *offscreenTarget, *swapchainRenderPass, *descriptorPool,
      *graphicsPipeline, ctx.pipelineMode, *timer);
  offscreenTarget->registerWithImGui(descriptorPool->pool());

  // renderer
  renderer = std::make_unique<render::Renderer>(
      *device, *swapchain, *commandPool, *syncObjects, *resourceManager, *ui);

  // camera
  camera = std::make_unique<scene::Camera>(
      *window, *timer, ctx.pipelineMode, ctx.cameraMovementSpeed,
      ctx.cameraMouseSensitivity, ctx.cameraFov, ctx.cameraAspectRatio,
      ctx.cameraNearPlane, ctx.cameraFarPlane);
  ctx.cameraLocked = camera->isLocked();

  onSetup();
}

void VulkanApplication::mainLoop() {
  timer->start();
  bool isLastTabKeyPressed = false;

  while (!window->shouldClose()) {
    timer->maxFPS(ctx.maxFPS);
    timer->beginFrame();

    window->pollEvents();
    graphicsPipeline->flushPendingRebuild();

    bool isNowTabKeyPressed =
        glfwGetKey(window->glfwWindow(), GLFW_KEY_TAB) == GLFW_PRESS;

    if (ctx.cameraEnabled) {
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

void VulkanApplication::recreateSwapchain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window->glfwWindow(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window->glfwWindow(), &width, &height);
    glfwWaitEvents();
  }
  renderer->recreateSwapchain();
}

void VulkanApplication::drawFrame() {
  render::FrameData frameData;
  if (!renderer->beginFrame(frameData)) {
    return;
  }

  if (offscreenTarget->flushPendingResize(descriptorPool->pool())) {
    graphicsPipeline->buildOffscreen(offscreenRenderPass->renderPass());
  }

  onDrawFrame(frameData.frameIndex);

  renderer->beginOffscreenPass(frameData,
                               *resourceManager->getFramebufferSet("offscreen"),
                               *offscreenRenderPass, *offscreenTarget);
  renderer->bindPipeline(frameData, graphicsPipeline->offscreenPipeline(),
                         graphicsPipeline->offscreenPipelineLayout(),
                         descriptorSets->sets());
  renderer->setOffscreenViewportAndScissor(frameData, *offscreenTarget);
  renderer->drawGeometry(frameData);
  renderer->endPass(frameData);

  renderer->beginSwapchainPass(frameData,
                               *resourceManager->getFramebufferSet("swapchain"),
                               *swapchainRenderPass);
  renderer->drawUI(frameData);
  renderer->endPass(frameData);

  renderer->endFrame(frameData);
}

void VulkanApplication::cleanup() {
  onCleanup();

  ui.reset();
  timer.reset();
  camera.reset();
  renderer.reset();
  syncObjects.reset();
  offscreenTarget.reset();
  resourceManager.reset();
  depthResources.reset();
  commandBuffers.reset();
  descriptorSets.reset();
  descriptorSetLayout.reset();
  descriptorManager.reset();
  descriptorPool.reset();
  commandPool.reset();
  graphicsPipeline.reset();
  swapchainRenderPass.reset();
  offscreenRenderPass.reset();
  swapchain.reset();
  device.reset();
  surface.reset();
  instance.reset();
  window.reset();
}

} // namespace vkr
