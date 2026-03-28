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
  renderPass = std::make_unique<pipeline::RenderPass>(*device, *swapchain);

  // resource manager
  depthResources = std::make_unique<resource::DepthResources>(
      *device, *swapchain, *commandPool);
  resourceManager = std::make_unique<resource::ResourceManager>(
      *device, *swapchain, *commandPool, *depthResources, *renderPass);
  resourceManager->createFramebuffers("swapchain");

  offscreenTarget = std::make_unique<resource::OffscreenTarget>(
      *device, *commandPool, swapchain->extent2D().width,
      swapchain->extent2D().height);

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
      *instance, *device, *resourceManager, *renderPass, *descriptorSetLayout,
      ctx.pipelineMode);
  graphicsPipeline->buildOffscreen(offscreenTarget->renderPass());

  // descriptor sets
  descriptorSets =
      descriptorManager->allocate(*descriptorSetLayout, *descriptorPool);
  bindDescriptorSets();

  // timer
  timer = std::make_unique<Timer>();

  // ui
  ui = std::make_unique<ui::UI>(*window, *instance, *surface, *device,
                                *commandPool, *renderPass, *descriptorPool,
                                *graphicsPipeline, *timer, ctx.pipelineMode,
                                offscreenTarget.get());
  offscreenTarget->registerWithImGui(descriptorPool->pool());

  // renderer
  renderer = std::make_unique<render::Renderer>(
      *device, *swapchain, *commandPool, *syncObjects, *resourceManager,
      *renderPass, *ui);

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
    onUpdate(timer->deltaTime());
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
    graphicsPipeline->buildOffscreen(offscreenTarget->renderPass());
  }

  updateUniforms(frameData.frameIndex);

  renderer->beginOffscreenPass(frameData, *offscreenTarget);
  renderer->bindPipeline(frameData, graphicsPipeline->offscreenPipeline(),
                         graphicsPipeline->offscreenPipelineLayout(),
                         descriptorSets->sets());
  renderer->setOffscreenViewportAndScissor(frameData, *offscreenTarget);
  renderer->drawGeometry(frameData);
  renderer->endOffscreenPass(frameData);

  renderer->beginRenderPass(frameData);
  renderer->drawUI(frameData);
  renderer->endRenderPass(frameData);

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
  renderPass.reset();
  swapchain.reset();
  device.reset();
  surface.reset();
  instance.reset();
  window.reset();
}

} // namespace vkr
