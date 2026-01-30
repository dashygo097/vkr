#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

namespace vkr {

void VulkanApplication::initVulkan() {
  // window
  window = std::make_unique<core::Window>(ctx.title, ctx.width, ctx.height);
  glfwSetWindowUserPointer(window->glfwWindow(), this);
  glfwSetFramebufferSizeCallback(window->glfwWindow(),
                                 framebufferResizeCallback);

  // instance
  instance = std::make_unique<core::Instance>(
      ctx.appName, ctx.engineName, ctx.appVersion, ctx.engineVersion,
      ctx.preExtensions, ctx.validationLayers);

  // surface
  surface = std::make_unique<core::Surface>(*window, *instance);

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

  // sync_objects
  syncObjects = std::make_unique<core::SyncObjects>(*device, *swapchain);

  // render pass
  renderPass = std::make_unique<pipeline::RenderPass>(*device, *swapchain);

  // resource manager
  resourceManager = std::make_unique<resource::ResourceManager>(
      *device, *commandPool, *swapchain, *renderPass);
  resourceManager->createFramebuffers("swapchain");

  createUniforms();

  // descriptor manager
  uint32_t maxSets = MAX_FRAMES_IN_FLIGHT + 1;
  pipeline::DescriptorPoolSizes poolSizes =
      pipeline::DescriptorManager::calculatePoolSizes(maxSets);
  descriptorPool =
      std::make_unique<pipeline::DescriptorPool>(*device, maxSets, poolSizes);

  descriptorManager = std::make_unique<pipeline::DescriptorManager>(*device);
  std::vector<pipeline::DescriptorBinding> bindings =
      createDescriptorBindings();

  descriptorSetLayout = descriptorManager->createLayout(bindings);

  // graphics pipeline
  graphicsPipeline = std::make_unique<pipeline::GraphicsPipeline>(
      *device, *resourceManager, *renderPass, *descriptorSetLayout,
      ctx.vertexShaderPath, ctx.fragmentShaderPath, ctx.pipelineMode);

  // descriptor sets
  descriptorSets =
      descriptorManager->allocate(*descriptorSetLayout, *descriptorPool);
  bindDescriptorSets();

  // renderer
  renderer = std::make_unique<render::Renderer>(*device, *swapchain,
                                                *commandPool, *syncObjects,
                                                *resourceManager, *renderPass);

  // camera
  camera = std::make_unique<scene::Camera>(
      *window, ctx.cameraMovementSpeed, ctx.cameraMouseSensitivity,
      ctx.cameraFov, ctx.cameraAspectRatio, ctx.cameraNearPlane,
      ctx.cameraFarPlane);
  ctx.cameraLocked = camera->isLocked();

  // timer
  timer = std::make_unique<Timer>();

  // ui
  ui = std::make_unique<ui::UI>(*window, *instance, *surface, *device,
                                *commandPool, *renderPass, *descriptorPool);
}

void VulkanApplication::mainLoop() {
  timer->start();
  bool isLastTabKeyPressed = false;

  while (!window->shouldClose()) {
    window->pollEvents();

    bool isNowTabKeyPressed =
        glfwGetKey(window->glfwWindow(), GLFW_KEY_TAB) == GLFW_PRESS;

    if (ctx.cameraEnabled) {
      camera->track();
      camera->lock(ui->layoutMode() == ui::LayoutMode::Standard);
    }

    if (isNowTabKeyPressed && !isLastTabKeyPressed) {
      ui->switchLayoutMode();
    }

    timer->update();
    onUpdate(timer->deltaTime());
    drawFrame();

    isLastTabKeyPressed = isNowTabKeyPressed;
    device->waitIdle();
  }
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

  const auto &viewport = ui->viewportInfo();

  updateUniforms(frameData.frameIndex);

  renderer->beginRenderPass(frameData);
  renderer->bindPipeline(frameData, graphicsPipeline->pipeline(),
                         graphicsPipeline->pipelineLayout(),
                         descriptorSets->sets());
  renderer->setViewportAndScissor(frameData);
  renderer->drawGeometry(frameData);
  renderer->drawUI(frameData, *ui);
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
  resourceManager.reset();
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
