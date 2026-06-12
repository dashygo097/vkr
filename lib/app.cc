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

  // window
  window = std::make_unique<core::Window>(ctx.window);
  glfwSetWindowUserPointer(window->glfwWindow(), this);
  glfwSetFramebufferSizeCallback(window->glfwWindow(),
                                 framebufferResizeCallback);

  // instance
  instance = std::make_unique<core::Instance>(ctx.instance);

  // surface
  surface = std::make_unique<core::Surface>(*instance, *window);

  // device
  device = std::make_unique<core::Device>(
      *instance, *surface, ctx.deviceExtensions, ctx.instance.validationLayers);

  // swapchain
  swapchain = std::make_unique<core::Swapchain>(*window, *device, *surface,
                                                ctx.presentModePolicy);

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

  // framebuffer set (swapchain)
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

  // render pass (offscreen)
  pipeline::RenderPassDesc offscreenRenderPassDesc =
      pipeline::makeOffscreenRenderPassDesc(VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_FORMAT_D32_SFLOAT);
  offscreenRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  offscreenRenderPass->update(offscreenRenderPassDesc);
  offscreenTarget = std::make_unique<resource::OffscreenTarget>(
      *device, *commandPool, *offscreenRenderPass);
  offscreenTarget->resize(swapchain->extent2D());

  // framebuffer set (offscreen)
  vkr::resource::FramebufferDesc offscreenTargetfbDesc{};
  offscreenTargetfbDesc.extent = offscreenTarget->extent2D();
  offscreenTargetfbDesc.layers = 1;
  offscreenTargetfbDesc.attachments = {
      {offscreenTarget->colorView(), offscreenTarget->depthView()},
  };

  resourceManager->createFramebufferSet("offscreen", *offscreenRenderPass,
                                        offscreenTargetfbDesc);

  createResources();

  // descriptor pool
  uint32_t maxSets = core::MAX_FRAMES_IN_FLIGHT + 2;

  pipeline::DescriptorPoolSizes descPoolSizes;
  descPoolSizes.uniformBufferCount = maxSets * 10;
  descPoolSizes.storageBufferCount = maxSets * 10;
  descPoolSizes.combinedImageSamplerCount = maxSets * 10;
  descPoolSizes.storageImageCount = maxSets * 10;
  descPoolSizes.inputAttachmentCount = maxSets * 10;

  descriptorPool = std::make_unique<pipeline::DescriptorPool>(*device, maxSets,
                                                              descPoolSizes);

  // descriptor layout
  descriptorSetLayout = std::make_shared<pipeline::DescriptorSetLayout>(
      *device, createDescriptorBindings());

  // graphics pipeline
  graphicsPipeline = std::make_unique<pipeline::GraphicsPipeline>(
      *instance, *device, *resourceManager, *swapchainRenderPass,
      *descriptorSetLayout, ctx.pipelineMode);
  graphicsPipeline->buildOffscreen(offscreenRenderPass->renderPass());

  // descriptor sets
  descriptorSets = std::make_unique<pipeline::DescriptorSets>(
      *device, *resourceManager, *descriptorSetLayout, *descriptorPool);
  descriptorSets->bindResources();

  // timer
  timer = std::make_unique<util::Timer>();

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
  camera = std::make_unique<scene::Camera>(*window, *timer, ctx.pipelineMode,
                                           ctx.camera);
}

void VulkanApplication::mainLoop() {
  timer->start();
  bool isLastTabKeyPressed = false;

  while (!window->shouldClose() && !ui->shouldClose()) {
    timer->beginFrame();

    window->pollEvents();

    graphicsPipeline->flushPendingRebuild();

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

} // namespace vkr
