#define GLM_FORCE_RADIANS
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

#include "app.hpp"
#include "impl/vertex.hpp"

class UniformBufferTestApplication : public VulkanApplication {
private:
  const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

  const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

  void initVulkan() {

    ctx.appName = "Hello Triangle";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);
    ctx.engineName = "No Engine";
    ctx.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    window = std::make_unique<Window>(ctx);
    ctx.window = window->getGLFWWindow();
    glfwSetWindowUserPointer(window->getGLFWWindow(), this);
    glfwSetFramebufferSizeCallback(window->getGLFWWindow(),
                                   framebufferResizeCallback);

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

    descriptorSetLayout = std::make_unique<DescriptorSetLayout>(ctx);
    ctx.descriptorSetLayout = descriptorSetLayout->getVkDescriptorSetLayout();

    ctx.vertexShaderPath = "shaders/uniform/vert.spv";
    ctx.fragmentShaderPath = "shaders/uniform/frag.spv";
    graphicsPipeline = std::make_unique<GraphicsPipeline>(ctx);
    ctx.graphicsPipeline = graphicsPipeline->getVkPipeline();
    ctx.pipelineLayout = graphicsPipeline->getVkPipelineLayout();

    swapchainFramebuffers = std::make_unique<Framebuffers>(ctx);
    ctx.swapchainFramebuffers = swapchainFramebuffers->getVkFramebuffers();

    commandPool = std::make_unique<CommandPool>(ctx);
    ctx.commandPool = commandPool->getVkCommandPool();

    vertexBuffer = std::make_unique<VertexBuffer>(vertices, ctx);
    ctx.vertexBuffer = vertexBuffer->getVkBuffer();
    ctx.vertexBufferMemory = vertexBuffer->getVkBufferMemory();

    indexBuffer = std::make_unique<IndexBuffer>(indices, ctx);
    ctx.indexBuffer = indexBuffer->getVkBuffer();
    ctx.indexBufferMemory = indexBuffer->getVkBufferMemory();

    uniformBuffers =
        std::make_unique<UniformBuffers>(UniformBufferObject{}, ctx);
    ctx.uniformBuffers = uniformBuffers->getVkBuffers();
    ctx.uniformBuffersMemory = uniformBuffers->getVkBuffersMemory();
    ctx.uniformBuffersMapped = uniformBuffers->getMapped();

    descriptorSet = std::make_unique<DescriptorSet>(ctx);
    ctx.descriptorSets = descriptorSet->getVkDescriptorSets();
    ctx.descriptorPool = descriptorSet->getVkDescriptorPool();

    commandBuffers = std::make_unique<CommandBuffers>(ctx);
    ctx.commandBuffers = commandBuffers->getVkCommandBuffersRef();

    syncObjects = std::make_unique<SyncObjects>(ctx);
    ctx.imageAvailableSemaphores = syncObjects->getImageAvailableSemaphores();
    ctx.renderFinishedSemaphores = syncObjects->getRenderFinishedSemaphores();
    ctx.inFlightFences = syncObjects->getInFlightFences();
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height) {
    auto app = reinterpret_cast<UniformBufferTestApplication *>(
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
    swapchainFramebuffers->destroy();
    swapchainFramebuffers->create();
  }

  void drawFrame() {
    vkWaitForFences(device->getVkDevice(), 1,
                    &syncObjects->getInFlightFences()[ctx.currentFrame],
                    VK_TRUE, UINT64_MAX);

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

    vkResetCommandBuffer(
        commandBuffers->getVkCommandBuffers()[ctx.currentFrame],
        /*VkCommandBufferResetFlagBits*/ 0);

    recordCommandBuffer(
        imageIndex, ctx.currentFrame, vertices, indices,
        indexBuffer->getVkBuffer(), vertexBuffer->getVkBuffer(),
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

  void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view =
        glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                ctx.swapchainExtent.width /
                                    (float)ctx.swapchainExtent.height,
                                0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    uniformBuffers->update(currentImage, ubo);
  }
};

int main() {
  UniformBufferTestApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
