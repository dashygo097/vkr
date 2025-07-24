#include "vkr/app.hpp"

namespace vkr {
VulkanApplication::VulkanApplication() {}
VulkanApplication::~VulkanApplication() { cleanup(); }

void VulkanApplication::configure() {}

void VulkanApplication::initVulkan() {
  window = std::make_unique<Window>(ctx);
  ctx.window = window->glfwWindow();
  glfwSetWindowUserPointer(window->glfwWindow(), this);
  glfwSetFramebufferSizeCallback(window->glfwWindow(),
                                 framebufferResizeCallback);

  camera = std::make_unique<Camera>(ctx);
  ctx.cameraMovementSpeed = camera->getMovementSpeed();
  ctx.cameraMouseSensitivity = camera->getMouseSensitivity();
  ctx.cameraFov = camera->getFov();
  ctx.cameraAspectRatio = camera->getAspectRatio();
  ctx.cameraNearPlane = camera->getNearPlane();
  ctx.cameraFarPlane = camera->getFarPlane();

  instance = std::make_unique<Instance>(ctx);
  ctx.instance = instance->instance();

  surface = std::make_unique<Surface>(ctx);
  ctx.surface = surface->surface();

  device = std::make_unique<Device>(ctx);
  ctx.device = device->device();
  ctx.physicalDevice = device->physicalDevice();
  ctx.graphicsQueue = device->graphicsQueue();
  ctx.presentQueue = device->presentQueue();

  swapchain = std::make_unique<Swapchain>(ctx);
  ctx.swapchain = swapchain->swapchain();
  ctx.swapchainImages = swapchain->images();
  ctx.swapchainImageViews = swapchain->imageViews();
  ctx.swapchainImageFormat = swapchain->format();
  ctx.swapchainExtent = swapchain->extent2D();

  renderPass = std::make_unique<RenderPass>(ctx);
  ctx.renderPass = renderPass->renderPass();

  swapchainFramebuffers = std::make_unique<Framebuffers>(ctx);
  ctx.swapchainFramebuffers = swapchainFramebuffers->framebuffers();

  descriptorSetLayout = std::make_unique<DescriptorSetLayout>(ctx);
  ctx.descriptorSetLayout = descriptorSetLayout->descriptorSetLayout();

  graphicsPipeline = std::make_unique<GraphicsPipeline>(ctx);
  ctx.pipelineLayout = graphicsPipeline->pipelineLayout();
  ctx.graphicsPipeline = graphicsPipeline->pipeline();

  commandPool = std::make_unique<CommandPool>(ctx);
  ctx.commandPool = commandPool->commandPool();

  vertexBuffers =
      std::make_unique<std::vector<std::unique_ptr<VertexBuffer>>>();
  indexBuffers = std::make_unique<std::vector<std::unique_ptr<IndexBuffer>>>();

  uniformBuffers = std::make_unique<UniformBuffers>(UniformBufferObject{}, ctx);
  ctx.uniformBuffers = uniformBuffers->buffers();
  ctx.uniformBuffersMemory = uniformBuffers->buffersMemory();
  ctx.uniformBuffersMapped = uniformBuffers->mapped();

  descriptorSet = std::make_unique<DescriptorSet>(ctx);
  ctx.descriptorSets = descriptorSet->descriptorSets();
  ctx.descriptorPool = descriptorSet->descriptorPool();

  commandBuffers = std::make_unique<CommandBuffers>(ctx);
  ctx.commandBuffers = commandBuffers->commandBuffers();

  syncObjects = std::make_unique<SyncObjects>(ctx);
  ctx.imageAvailableSemaphores = syncObjects->imageAvailableSemaphores();
  ctx.renderFinishedSemaphores = syncObjects->renderFinishedSemaphores();
  ctx.inFlightFences = syncObjects->inFlightFences();

  fpsCounter = std::make_unique<FPSCounter>();
  ui = std::make_unique<UI>(ctx);
}

void VulkanApplication::setting() {}

void VulkanApplication::mainLoop() {
  while (!window->shouldClose()) {
    window->pollEvents();
    if (ctx.cameraEnabled) {
      camera->track();
    }
    drawFrame();
  }

  device->waitIdle();
}

void VulkanApplication::cleanup() {
  ui.reset();
  fpsCounter.reset();
  syncObjects.reset();
  commandBuffers.reset();
  descriptorSet.reset();
  uniformBuffers.reset();
  indexBuffers.reset();
  vertexBuffers.reset();
  commandPool.reset();
  graphicsPipeline.reset();
  descriptorSetLayout.reset();
  swapchainFramebuffers.reset();
  renderPass.reset();
  swapchain.reset();
  device.reset();
  surface.reset();
  instance.reset();
  camera.reset();
  window.reset();
}

void VulkanApplication::recreateSwapchain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window->glfwWindow(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window->glfwWindow(), &width, &height);
    glfwWaitEvents();
  }

  swapchain->recreate();
  swapchainFramebuffers->destroy();
  swapchainFramebuffers->create();
}

void VulkanApplication::updateUniformBuffer(uint32_t currentImage) {
  UniformBufferObject ubo{};
  ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  ubo.view = camera->getView();
  ubo.proj = camera->getProjection();

  uniformBuffers->update(currentImage, ubo);
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

  updateUniformBuffer(ctx.currentFrame);

  vkResetFences(device->device(), 1,
                &syncObjects->inFlightFences()[ctx.currentFrame]);

  vkResetCommandBuffer(commandBuffers->commandBuffers()[ctx.currentFrame],
                       /*VkCommandBufferResetFlagBits*/ 0);

  // FIXME: This func is BAD.
  recordCommandBuffer(
      imageIndex, ctx.currentFrame,
      commandBuffers->commandBuffers()[ctx.currentFrame],
      renderPass->renderPass(), graphicsPipeline->pipelineLayout(),
      descriptorSet->descriptorSets(), swapchainFramebuffers->framebuffers(),
      swapchain->extent2D(), graphicsPipeline->pipeline(), *vertexBuffers,
      *indexBuffers, *ui);

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
} // namespace vkr
