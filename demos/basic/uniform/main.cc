#define GLM_FORCE_RADIANS
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

  void configure() {

    ctx.appName = "Vulkan App";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);
    ctx.engineName = "No Engine";
    ctx.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    ctx.width = 800;
    ctx.height = 600;
    std::string title = "Vulkan (Default Title)";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.vertexShaderPath = "shaders/uniform/vert.spv";
    ctx.fragmentShaderPath = "shaders/uniform/frag.spv";
  }

  void setting() {

    vertexBuffer->update(vertices);
    indexBuffer->update(indices);
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
