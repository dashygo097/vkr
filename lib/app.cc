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
  glfwSetFramebufferSizeCallback(window->glfwWindow(),
                                 framebufferResizeCallback);

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
  commandPool = std::make_unique<core::CommandPool>(*surface, *device);

  // command buffers
  commandBuffers =
      std::make_unique<core::CommandBuffers>(*device, *commandPool);

  // sync objects
  syncObjects = std::make_unique<core::SyncObjects>(*device, *swapchain);

  // resource manager
  depthTarget = std::make_unique<resource::DepthTarget>(*device, *commandPool);
  depthTarget->update(resource::DepthTargetDesc{
      .width = swapchain->extent2D().width,
      .height = swapchain->extent2D().height,
      .format = vkr::resource::findDepthFormat(device->physicalDevice())});

  resourceManager = std::make_unique<resource::ResourceManager>(
      *device, *swapchain, *commandPool);

  // render pass: swapchain
  swapchainRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  swapchainRenderPass->update(pipeline::RenderPassDesc::makeSwapchain(
      swapchain->format(),
      vkr::resource::findDepthFormat(device->physicalDevice())));

  // framebuffer set: swapchain
  resource::FramebufferDesc swapchainFbDesc{};
  swapchainFbDesc.extent = swapchain->extent2D();
  swapchainFbDesc.layers = 1;
  swapchainFbDesc.attachments.reserve(swapchain->images().size());

  for (auto imageView : swapchain->imageViews()) {
    swapchainFbDesc.attachments.push_back(
        {imageView, depthTarget->imageView()});
  }

  resourceManager->createFramebufferSet("swapchain", *swapchainRenderPass,
                                        swapchainFbDesc);

  // render pass: offscreen
  offscreenRenderPass = std::make_unique<pipeline::RenderPass>(*device);
  offscreenRenderPass->update(pipeline::RenderPassDesc::makeOffscreen(
      VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT));

  // offscreen target
  offscreenTarget = std::make_unique<resource::OffscreenTarget>(
      *device, *commandPool, *offscreenRenderPass);
  offscreenTarget->resize(swapchain->extent2D());

  // framebuffer set: offscreen
  resource::FramebufferDesc offscreenFbDesc{};
  offscreenFbDesc.extent = offscreenTarget->extent2D();
  offscreenFbDesc.layers = 1;
  offscreenFbDesc.attachments = {
      {offscreenTarget->colorView(), offscreenTarget->depthView()},
  };

  resourceManager->createFramebufferSet("offscreen", *offscreenRenderPass,
                                        offscreenFbDesc);

  // user resources
  createResources();

  // descriptor pool
  uint32_t maxSets = core::MAX_FRAMES_IN_FLIGHT + 2;

  pipeline::DescriptorPoolSizes descPoolSizes{};
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

  // pipeline library
  pipelineLibrary = std::make_unique<render::PipelineLibrary>(*device);
  createPipelines();

  if (pipelineLibrary->empty()) {
    VKR_CORE_WARN("No graphics pipelines were created");
  }

  // descriptor sets
  descriptorSets = std::make_unique<pipeline::DescriptorSets>(
      *device, *resourceManager, *descriptorSetLayout, *descriptorPool);
  descriptorSets->bindResources();

  // timer
  timer = std::make_unique<util::Timer>();

  // ui
  ui = std::make_unique<ui::UI>(*window, *instance, *surface, *device,
                                *commandPool, *resourceManager,
                                *offscreenTarget, *swapchainRenderPass,
                                *descriptorPool, *timer, ctx.theme);

  offscreenTarget->registerWithImGui(descriptorPool->pool());

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
  render::FrameData frameData{};
  if (!renderer->beginFrame(frameData)) {
    return;
  }

  offscreenTarget->flushPendingResize(descriptorPool->pool());

  onDrawFrame(frameData.frameIndex);

  renderer->beginOffscreenPass(frameData,
                               *resourceManager->getFramebufferSet("offscreen"),
                               *offscreenRenderPass, *offscreenTarget);

  renderer->setOffscreenViewportAndScissor(frameData, *offscreenTarget);

  if (!pipelineLibrary->empty()) {
    auto &pipeline = pipelineLibrary->first();
    renderer->bindPipeline(frameData, pipeline.pipeline(), pipeline.layout(),
                           descriptorSets->sets());
    renderer->drawGeometry(frameData);
  }

  renderer->endPass(frameData);

  renderer->beginSwapchainPass(frameData,
                               *resourceManager->getFramebufferSet("swapchain"),
                               *swapchainRenderPass);

  renderer->drawUI(frameData);
  renderer->endPass(frameData);

  renderer->endFrame(frameData);
}

} // namespace vkr
