#include "app.hpp"

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

  graphicsPipeline = std::make_unique<GraphicsPipeline>(ctx);
  ctx.pipelineLayout = graphicsPipeline->getVkPipelineLayout();
  ctx.graphicsPipeline = graphicsPipeline->getVkPipeline();

  commandPool = std::make_unique<CommandPool>(ctx);
  ctx.commandPool = commandPool->getVkCommandPool();

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
  commandPool.reset();
  graphicsPipeline.reset();
  swapchainFramebuffers.reset();
  renderPass.reset();
  swapchain.reset();
  device.reset();
  surface.reset();
  instance.reset();
  window.reset();
}

void VulkanApplication::drawFrame() {}
