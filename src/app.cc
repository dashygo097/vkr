#include "app.hpp"
#include <memory>

VulkanApplication::VulkanApplication() {}
VulkanApplication::~VulkanApplication() { cleanup(); }

void VulkanApplication::initVulkan() {
  window = std::make_unique<Window>(ctx);
  ctx.window = window->getGLFWWindow();

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

  descriptor = std::make_unique<Descriptor>(ctx);
  ctx.descriptorSetLayout = descriptor->getVkDescriptorSetLayout();

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

  commandBuffers = std::make_unique<CommandBuffers>(ctx);
  ctx.commandBuffers = commandBuffers->getVkCommandBuffers();

  syncObjects = std::make_unique<SyncObjects>(ctx);
  ctx.imageAvailableSemaphores = syncObjects->getImageAvailableSemaphores();
  ctx.renderFinishedSemaphores = syncObjects->getRenderFinishedSemaphores();
  ctx.inFlightFences = syncObjects->getInFlightFences();
}

void VulkanApplication::mainLoop() {
  while (!window->shouldClose()) {
    window->pollEvents();
    drawFrame();
  }

  device->waitIdle();
}

void VulkanApplication::cleanup() {
  syncObjects.reset();
  commandBuffers.reset();
  uniformBuffers.reset();
  indexBuffer.reset();
  vertexBuffer.reset();
  commandPool.reset();
  graphicsPipeline.reset();
  descriptor.reset();
  swapchainFramebuffers.reset();
  renderPass.reset();
  swapchain.reset();
  device.reset();
  surface.reset();
  instance.reset();
  window.reset();
}

void VulkanApplication::drawFrame() {
  // TODO: Implement the frame drawing logic
}
