#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <unistd.h>

namespace vkr {

void VulkanApplication::initVulkan() {
  // window
  window = std::make_unique<Window>(ctx.title, ctx.width, ctx.height);
  glfwSetWindowUserPointer(window->glfwWindow(), this);
  glfwSetFramebufferSizeCallback(window->glfwWindow(),
                                 framebufferResizeCallback);

  // instance
  instance = std::make_unique<Instance>(
      ctx.appName, ctx.engineName, ctx.appVersion, ctx.engineVersion,
      ctx.preExtensions, ctx.validationLayers);

  // surface
  surface = std::make_unique<Surface>(*window, *instance);

  // device
  device = std::make_unique<Device>(*instance, *surface, ctx.deviceExtensions,
                                    ctx.validationLayers);

  // swapchain
  swapchain = std::make_unique<Swapchain>(*window, *device, *surface);

  // command pool
  commandPool = std::make_unique<CommandPool>(*device, *surface);

  // command buffers
  commandBuffers = std::make_unique<CommandBuffers>(*device, *commandPool);

  // sync_objects
  syncObjects = std::make_unique<SyncObjects>(*device, *swapchain);

  // render pass
  renderPass = std::make_unique<RenderPass>(*device, *swapchain);

  // resource manager
  resourceManager = std::make_unique<ResourceManager>(*device, *commandPool,
                                                      *renderPass, *swapchain);
  resourceManager->createFramebuffers("swapchain");

  createUniforms();
  // for (auto uniform : resourceManager->listUniformBuffers()) {
  //   for (auto buffer : uniform->buffers()) {
  //     ctx.uniformBuffers.push_back(buffer);
  //   }
  //   for (auto memory : uniform->buffersMemory()) {
  //     ctx.uniformBuffersMemory.push_back(memory);
  //   }
  //   for (auto mapped : uniform->mapped()) {
  //     ctx.uniformBuffersMapped.push_back(mapped);
  //   }
  // }

  // descriptor manager
  uint32_t maxSets = MAX_FRAMES_IN_FLIGHT + 1;
  DescriptorPoolSizes poolSizes =
      DescriptorManager::calculatePoolSizes(maxSets);
  descriptorPool =
      std::make_unique<DescriptorPool>(*device, maxSets, poolSizes);

  descriptorManager = std::make_unique<DescriptorManager>(*device);
  std::vector<DescriptorBinding> bindings = createDescriptorBindings();

  descriptorSetLayout = descriptorManager->createLayout(bindings);

  // graphics pipeline
  graphicsPipeline = std::make_unique<GraphicsPipeline>(
      *device, *renderPass, *descriptorSetLayout, ctx.vertexShaderPath,
      ctx.fragmentShaderPath, ctx.pipelineMode);

  // descriptor sets
  descriptorSets =
      descriptorManager->allocate(*descriptorSetLayout, *descriptorPool);
  bindDescriptorSets();

  // camera
  camera = std::make_unique<Camera>(*window, ctx.cameraMovementSpeed,
                                    ctx.cameraMouseSensitivity, ctx.cameraFov,
                                    ctx.cameraAspectRatio, ctx.cameraNearPlane,
                                    ctx.cameraFarPlane);
  ctx.cameraLocked = camera->isLocked();

  // timer
  timer = std::make_unique<Timer>();

  // ui
  ui = std::make_unique<UI>(*window, *instance, *surface, *device, *renderPass,
                            *descriptorPool, *commandPool);
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
      camera->lock(ui->isVisible());
    }

    if (isNowTabKeyPressed && !isLastTabKeyPressed) {
      ui->toggleVisibility();
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
      &commandBuffers->commandBuffers()[ctx.currentFrame];

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
