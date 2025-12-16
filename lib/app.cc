#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <unistd.h>

namespace vkr {

void VulkanApplication::initVulkan() {
  // window
  window = std::make_unique<Window>(ctx);
  ctx.window = window->glfwWindow();
  glfwSetWindowUserPointer(window->glfwWindow(), this);
  glfwSetFramebufferSizeCallback(window->glfwWindow(),
                                 framebufferResizeCallback);

  // instance
  instance = std::make_unique<Instance>(ctx);
  ctx.instance = instance->instance();

  // surface
  surface = std::make_unique<Surface>(ctx);
  ctx.surface = surface->surface();

  // device
  device = std::make_unique<Device>(ctx);
  ctx.device = device->device();
  ctx.physicalDevice = device->physicalDevice();
  ctx.graphicsQueue = device->graphicsQueue();
  ctx.presentQueue = device->presentQueue();

  // swapchain
  swapchain = std::make_unique<Swapchain>(ctx);
  ctx.swapchain = swapchain->swapchain();
  ctx.swapchainImages = swapchain->images();
  ctx.swapchainImageViews = swapchain->imageViews();
  ctx.swapchainImageFormat = swapchain->format();
  ctx.swapchainExtent = swapchain->extent2D();

  // command pool
  commandPool = std::make_unique<CommandPool>(ctx);
  ctx.commandPool = commandPool->commandPool();

  // command buffers
  commandBuffers = std::make_unique<CommandBuffers>(ctx);
  ctx.commandBuffers = commandBuffers->commandBuffers();

  // sync_objects
  // TODO: split sync_objects into semaphore and fence
  syncObjects = std::make_unique<SyncObjects>(ctx);
  ctx.imageAvailableSemaphores = syncObjects->imageAvailableSemaphores();
  ctx.renderFinishedSemaphores = syncObjects->renderFinishedSemaphores();
  ctx.inFlightFences = syncObjects->inFlightFences();

  // render pass
  renderPass = std::make_unique<RenderPass>(ctx);
  ctx.renderPass = renderPass->renderPass();

  // resource manager
  resourceManager = std::make_unique<ResourceManager>(ctx);
  resourceManager->createFramebuffers("swapchain");
  ctx.swapchainFramebuffers =
      resourceManager->getFramebuffers("swapchain")->framebuffers();

  createUniforms();
  for (auto uniform : resourceManager->listUniformBuffers()) {
    for (auto buffer : uniform->buffers()) {
      ctx.uniformBuffers.push_back(buffer);
    }
    for (auto memory : uniform->buffersMemory()) {
      ctx.uniformBuffersMemory.push_back(memory);
    }
    for (auto mapped : uniform->mapped()) {
      ctx.uniformBuffersMapped.push_back(mapped);
    }
  }

  // descriptor manager
  uint32_t maxSets = MAX_FRAMES_IN_FLIGHT + 1;
  DescriptorPoolSizes poolSizes =
      DescriptorManager::calculatePoolSizes(maxSets);
  descriptorPool = std::make_unique<DescriptorPool>(ctx, maxSets, poolSizes);
  ctx.descriptorPool = descriptorPool->pool();

  descriptorManager = std::make_unique<DescriptorManager>(ctx);
  std::vector<DescriptorBinding> bindings = createDescriptorBindings();

  descriptorSetLayout = descriptorManager->createLayout(bindings);
  ctx.descriptorSetLayout = descriptorSetLayout->layout();

  // graphics pipeline
  graphicsPipeline = std::make_unique<GraphicsPipeline>(ctx);
  ctx.pipelineLayout = graphicsPipeline->pipelineLayout();
  ctx.graphicsPipeline = graphicsPipeline->pipeline();

  // descriptor sets
  descriptorSets = descriptorManager->allocate(descriptorSetLayout->layout(),
                                               descriptorPool->pool());
  bindDescriptorSets();
  ctx.descriptorSets = descriptorSets->sets();

  // camera
  camera = std::make_unique<Camera>(ctx);
  ctx.cameraLocked = camera->isLocked();
  ctx.cameraMovementSpeed = camera->movementSpeed();
  ctx.cameraMouseSensitivity = camera->mouseSensitivity();
  ctx.cameraFov = camera->fov();
  ctx.cameraAspectRatio = camera->aspectRatio();
  ctx.cameraNearPlane = camera->nearPlane();
  ctx.cameraFarPlane = camera->farPlane();

  // timer
  timer = std::make_unique<Timer>();

  // ui
  ui = std::make_unique<UI>(ctx);
}

void VulkanApplication::mainLoop() {
  timer->start();
  bool isLastTabKeyPressed = false, isNowTabKeyPressed = false;

  while (!window->shouldClose()) {
    window->pollEvents();

    bool isNowTabKeyPressed =
        glfwGetKey(window->glfwWindow(), GLFW_KEY_TAB) == GLFW_PRESS;

    if (ctx.cameraEnabled) {
      camera->track();
      camera->lock(ui->isVisible());
      if (isNowTabKeyPressed && !isLastTabKeyPressed) {
        ui->toggleVisibility();
      }
      timer->update();
      onUpdate(timer->deltaTime());
      drawFrame();
    }

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

  swapchain->recreate();
  resourceManager->getFramebuffers("swapchain")->destroy();
  resourceManager->getFramebuffers("swapchain")->create();
}

void VulkanApplication::drawFrame() {
  vkWaitForFences(device->device(), 1,
                  &syncObjects->inFlightFences()[ctx.currentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device->device(), swapchain->swapchain(), UINT64_MAX,
      syncObjects->imageAvailableSemaphores()[ctx.currentFrame], VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  updateUniforms(ctx.currentFrame);

  vkResetFences(device->device(), 1,
                &syncObjects->inFlightFences()[ctx.currentFrame]);

  vkResetCommandBuffer(commandBuffers->commandBuffers()[ctx.currentFrame],
                       /*VkCommandBufferResetFlagBits*/ 0);

  commandBuffers->record(
      imageIndex, ctx.currentFrame, renderPass->renderPass(),
      graphicsPipeline->pipelineLayout(), descriptorSets->sets(),
      resourceManager->getFramebuffers("swapchain")->framebuffers(),
      swapchain->extent2D(), graphicsPipeline->pipeline(),
      resourceManager->listVertexBuffers(), resourceManager->listIndexBuffers(),
      *ui);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      syncObjects->imageAvailableSemaphores()[ctx.currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers =
      &commandBuffers->commandBuffersRef()[ctx.currentFrame];

  VkSemaphore signalSemaphores[] = {
      syncObjects->renderFinishedSemaphores()[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(device->graphicsQueue(), 1, &submitInfo,
                    syncObjects->inFlightFences()[ctx.currentFrame]) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {swapchain->swapchain()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(device->presentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      ctx.framebufferResized) {
    ctx.framebufferResized = false;
    recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
  ctx.currentFrame = (ctx.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanApplication::cleanup() {
  onCleanup();

  ui.reset();
  timer.reset();
  camera.reset();
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
