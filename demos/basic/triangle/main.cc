#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan_beta.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "interface/command_buffers.hpp"
#include "interface/command_pool.hpp"
#include "interface/device.hpp"
#include "interface/frame_buffers.hpp"
#include "interface/instance.hpp"
#include "interface/pipeline.hpp"
#include "interface/render_pass.hpp"
#include "interface/surface.hpp"
#include "interface/swapchain.hpp"
#include "interface/sync_objects.hpp"
#include "interface/window.hpp"
#include <ctx.hpp>

class HelloTriangleApplication {
public:
  void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  VulkanContext ctx;
  std::unique_ptr<Window> window;
  std::unique_ptr<Instance> instance;
  std::unique_ptr<Surface> surface;
  std::unique_ptr<Device> device;

  std::unique_ptr<Swapchain> swapchain;
  std::unique_ptr<Framebuffers> swapchainFrameBuffers;

  std::unique_ptr<RenderPass> renderPass;
  std::unique_ptr<GraphicsPipeline> graphicsPipeline;

  std::unique_ptr<CommandPool> commandPool;
  std::unique_ptr<CommandBuffers> commandBuffers;

  std::unique_ptr<SyncObjects> syncObjects;

  void initVulkan() {

    ctx.appName = (char *)"Hello Triangle";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);
    ctx.engineName = (char *)"No Engine";
    ctx.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    window = std::make_unique<Window>(ctx);
    glfwSetWindowUserPointer(window->getGLFWWindow(), this);
    glfwSetFramebufferSizeCallback(window->getGLFWWindow(),
                                   framebufferResizeCallback);
    ctx.window = window->getGLFWWindow();

    instance = std::make_unique<Instance>(ctx);
    ctx.instance = instance->getVkInstance();

    surface = std::make_unique<Surface>(ctx);
    ctx.surface = surface->getSurface();

    device = std::make_unique<Device>(ctx);
    ctx.device = device->getVkDevice();
    ctx.physicalDevice = device->getVkPhysicalDevice();
    ctx.graphicsQueue = device->getGraphicsQueue();
    ctx.presentQueue = device->getPresentQueue();

    swapchain = std::make_unique<Swapchain>(ctx);
    ctx.swapchain = swapchain->getSwapchain();
    ctx.swapchainImages = swapchain->getImages();
    ctx.swapchainImageViews = swapchain->getImageViews();
    ctx.swapchainImageFormat = swapchain->getFormat();
    ctx.swapchainExtent = swapchain->getExtent();

    renderPass = std::make_unique<RenderPass>(ctx);
    ctx.renderPass = renderPass->getRenderPass();

    ctx.vertexShaderPath = (char *)"shaders/triangle/vert.spv";
    ctx.fragmentShaderPath = (char *)"shaders/triangle/frag.spv";
    graphicsPipeline = std::make_unique<GraphicsPipeline>(ctx);
    ctx.graphicsPipeline = graphicsPipeline->getPipeline();
    ctx.pipelineLayout = graphicsPipeline->getPipelineLayout();

    swapchainFrameBuffers = std::make_unique<Framebuffers>(ctx);
    ctx.swapchainFramebuffers = swapchainFrameBuffers->getFramebuffers();

    commandPool = std::make_unique<CommandPool>(ctx);
    ctx.commandPool = commandPool->getCommandPool();

    commandBuffers = std::make_unique<CommandBuffers>(ctx);
    ctx.commandBuffers = commandBuffers->getCommandBuffersRef();

    syncObjects = std::make_unique<SyncObjects>(ctx);
    ctx.imageAvailableSemaphores = syncObjects->getImageAvailableSemaphores();
    ctx.renderFinishedSemaphores = syncObjects->getRenderFinishedSemaphores();
    ctx.inFlightFences = syncObjects->getInFlightFences();
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height) {
    auto app = reinterpret_cast<HelloTriangleApplication *>(
        glfwGetWindowUserPointer(window));
    app->ctx.framebufferResized = true;
  }

  void mainLoop() {
    while (!window->shouldClose()) {
      window->pollEvents();
      drawFrame();
    }

    device->waitIdle();
  }

  void cleanup() {}

  void recreateSwapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window->getGLFWWindow(), &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window->getGLFWWindow(), &width, &height);
      glfwWaitEvents();
    }

    swapchain->recreate();
    swapchainFrameBuffers->destroy();
    swapchainFrameBuffers->create();

    ctx.swapchain = swapchain->getSwapchain();
    ctx.swapchainImages = swapchain->getImages();
    ctx.swapchainImageViews = swapchain->getImageViews();
    ctx.swapchainImageFormat = swapchain->getFormat();
    ctx.swapchainExtent = swapchain->getExtent();
    ctx.swapchainFramebuffers = swapchainFrameBuffers->getFramebuffers();
  }

  void drawFrame() {
    vkWaitForFences(device->getVkDevice(), 1,
                    &syncObjects->getInFlightFences()[ctx.currentFrame],
                    VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device->getVkDevice(), swapchain->getSwapchain(), UINT64_MAX,
        syncObjects->getImageAvailableSemaphores()[ctx.currentFrame],
        VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapchain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(device->getVkDevice(), 1,
                  &syncObjects->getInFlightFences()[ctx.currentFrame]);

    vkResetCommandBuffer(commandBuffers->getCommandBuffers()[ctx.currentFrame],
                         /*VkCommandBufferResetFlagBits*/ 0);

    recordCommandBuffer(
        imageIndex, commandBuffers->getCommandBuffers()[ctx.currentFrame],
        renderPass->getRenderPass(), swapchainFrameBuffers->getFramebuffers(),
        swapchain->getExtent(), graphicsPipeline->getPipeline());

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
        &commandBuffers->getCommandBuffersRef()[ctx.currentFrame];

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

    VkSwapchainKHR swapchains[] = {swapchain->getSwapchain()};
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
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
