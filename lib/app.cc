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
  pipeline::RenderPassDesc desc{};

  pipeline::RenderPassColorAttachmentDesc color{};
  color.format = swapchain->format();
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  color.subpassLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  desc.colors.push_back(color);

  desc.depth.enabled = true;
  desc.depth.format = resource::findDepthFormat(device->physicalDevice());
  desc.depth.samples = VK_SAMPLE_COUNT_1_BIT;
  desc.depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  desc.depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  desc.depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  desc.depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  desc.depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  desc.depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  desc.depth.subpassLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  desc.dependencies.push_back(dependency);
  swapchainRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  swapchainRenderPass->update(desc);

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

  offscreenTarget = std::make_unique<resource::OffscreenTarget>(
      *device, *commandPool, swapchain->extent2D());

  vkr::resource::FramebufferDesc offscreenTargetDesc{};
  offscreenTargetDesc.extent = offscreenTarget->extent2D();
  offscreenTargetDesc.layers = 1;
  offscreenTargetDesc.attachments = {
      {offscreenTarget->colorView(), offscreenTarget->depthView()},
  };

  // resourceManager->createFramebufferSet("offscreen", offscreenTargetDesc);

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
  graphicsPipeline->buildOffscreen(offscreenTarget->renderPass());

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
      *device, *swapchain, *commandPool, *syncObjects, *resourceManager,
      *swapchainRenderPass, *ui);

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
  swapchainRenderPass.reset();
  swapchain.reset();
  device.reset();
  surface.reset();
  instance.reset();
  window.reset();
}

} // namespace vkr
