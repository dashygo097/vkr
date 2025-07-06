#include "vkr/app.hpp"
#include <memory>

VulkanApplication::VulkanApplication() {}
VulkanApplication::~VulkanApplication() { cleanup(); }

void VulkanApplication::configure() {}

void VulkanApplication::initVulkan() {
  window = std::make_unique<Window>(ctx);
  ctx.window = window->getGLFWWindow();
  glfwSetWindowUserPointer(window->getGLFWWindow(), this);
  glfwSetFramebufferSizeCallback(window->getGLFWWindow(),
                                 framebufferResizeCallback);

  camera = std::make_unique<Camera>(ctx);
  ctx.cameraMovementSpeed = camera->getMovementSpeed();
  ctx.cameraMouseSensitivity = camera->getMouseSensitivity();
  ctx.cameraFov = camera->getFov();
  ctx.cameraAspectRatio = camera->getAspectRatio();
  ctx.cameraNearPlane = camera->getNearPlane();
  ctx.cameraFarPlane = camera->getFarPlane();

  instance = std::make_unique<Instance>(ctx);
  ctx.instance = instance->getVkInstance();

  surface = std::make_unique<Surface>(ctx);
  ctx.surface = surface->getVkSurface();

  device = std::make_unique<Device>(ctx);
  ctx.device = device->getVkDevice();
  ctx.physicalDevice = device->getVkPhysicalDevice();
  ctx.graphicsQueue = device->getGraphicsQueue();
  ctx.presentQueue = device->getPresentQueue();

  swapchain = std::make_unique<Swapchain>(ctx);
  ctx.swapchain = swapchain->getVkSwapchain();
  ctx.swapchainImages = swapchain->getVkImages();
  ctx.swapchainImageViews = swapchain->getVkImageViews();
  ctx.swapchainImageFormat = swapchain->getVkFormat();
  ctx.swapchainExtent = swapchain->getVkExtent2D();

  renderPass = std::make_unique<RenderPass>(ctx);
  ctx.renderPass = renderPass->getVkRenderPass();

  swapchainFramebuffers = std::make_unique<Framebuffers>(ctx);
  ctx.swapchainFramebuffers = swapchainFramebuffers->getVkFramebuffers();

  descriptorSetLayout = std::make_unique<DescriptorSetLayout>(ctx);
  ctx.descriptorSetLayout = descriptorSetLayout->getVkDescriptorSetLayout();

  graphicsPipeline = std::make_unique<GraphicsPipeline>(ctx);
  ctx.pipelineLayout = graphicsPipeline->getVkPipelineLayout();
  ctx.graphicsPipeline = graphicsPipeline->getVkPipeline();

  commandPool = std::make_unique<CommandPool>(ctx);
  ctx.commandPool = commandPool->getVkCommandPool();

  vertexBuffer = std::make_unique<VertexBuffer>(std::vector<Vertex>{}, ctx);
  ctx.vertexBuffer = vertexBuffer->getVkBuffer();
  ctx.vertexBufferMemory = vertexBuffer->getVkBufferMemory();

  indexBuffer = std::make_unique<IndexBuffer>(std::vector<uint16_t>{}, ctx);
  ctx.indexBuffer = indexBuffer->getVkBuffer();
  ctx.indexBufferMemory = indexBuffer->getVkBufferMemory();

  uniformBuffers = std::make_unique<UniformBuffers>(UniformBufferObject{}, ctx);
  ctx.uniformBuffers = uniformBuffers->getVkBuffers();
  ctx.uniformBuffersMemory = uniformBuffers->getVkBuffersMemory();
  ctx.uniformBuffersMapped = uniformBuffers->getMapped();

  descriptorSet = std::make_unique<DescriptorSet>(ctx);
  ctx.descriptorSets = descriptorSet->getVkDescriptorSets();
  ctx.descriptorPool = descriptorSet->getVkDescriptorPool();

  commandBuffers = std::make_unique<CommandBuffers>(ctx);
  ctx.commandBuffers = commandBuffers->getVkCommandBuffers();

  syncObjects = std::make_unique<SyncObjects>(ctx);
  ctx.imageAvailableSemaphores = syncObjects->getImageAvailableSemaphores();
  ctx.renderFinishedSemaphores = syncObjects->getRenderFinishedSemaphores();
  ctx.inFlightFences = syncObjects->getInFlightFences();

  fps_counter = std::make_unique<FPSCounter>();
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
  fps_counter.reset();
  syncObjects.reset();
  commandBuffers.reset();
  descriptorSet.reset();
  uniformBuffers.reset();
  indexBuffer.reset();
  vertexBuffer.reset();
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
  glfwGetFramebufferSize(window->getGLFWWindow(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window->getGLFWWindow(), &width, &height);
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
  vkWaitForFences(device->getVkDevice(), 1,
                  &syncObjects->getInFlightFences()[ctx.currentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device->getVkDevice(), swapchain->getVkSwapchain(), UINT64_MAX,
      syncObjects->getImageAvailableSemaphores()[ctx.currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  updateUniformBuffer(ctx.currentFrame);

  vkResetFences(device->getVkDevice(), 1,
                &syncObjects->getInFlightFences()[ctx.currentFrame]);

  vkResetCommandBuffer(commandBuffers->getVkCommandBuffers()[ctx.currentFrame],
                       /*VkCommandBufferResetFlagBits*/ 0);

  recordCommandBuffer(
      imageIndex, ctx.currentFrame, vertexBuffer->getVertices(),
      indexBuffer->getIndices(), indexBuffer->getVkBuffer(),
      vertexBuffer->getVkBuffer(),
      commandBuffers->getVkCommandBuffers()[ctx.currentFrame],
      renderPass->getVkRenderPass(), graphicsPipeline->getVkPipelineLayout(),
      descriptorSet->getVkDescriptorSets(),
      swapchainFramebuffers->getVkFramebuffers(), swapchain->getVkExtent2D(),
      graphicsPipeline->getVkPipeline());

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      syncObjects->getImageAvailableSemaphores()[ctx.currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers =
      &commandBuffers->getVkCommandBuffersRef()[ctx.currentFrame];

  VkSemaphore signalSemaphores[] = {
      syncObjects->getRenderFinishedSemaphores()[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo,
                    syncObjects->getInFlightFences()[ctx.currentFrame]) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {swapchain->getVkSwapchain()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      ctx.framebufferResized) {
    ctx.framebufferResized = false;
    recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
  ctx.currentFrame = (ctx.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
