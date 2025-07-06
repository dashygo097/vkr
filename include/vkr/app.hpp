#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "./buffers/command.hpp"
#include "./buffers/frame.hpp"
#include "./buffers/index.hpp"
#include "./buffers/uniform.hpp"
#include "./buffers/vertex.hpp"
#include "./camera.hpp"
#include "./ctx.hpp"
#include "./interface/command_pool.hpp"
#include "./interface/descriptor_layout.hpp"
#include "./interface/descriptor_set.hpp"
#include "./interface/device.hpp"
#include "./interface/instance.hpp"
#include "./interface/pipeline.hpp"
#include "./interface/render_pass.hpp"
#include "./interface/surface.hpp"
#include "./interface/swapchain.hpp"
#include "./interface/sync_objects.hpp"
#include "./interface/window.hpp"

class VulkanApplication {
public:
  VulkanApplication();
  ~VulkanApplication();

  VulkanApplication(const VulkanApplication &) = delete;
  VulkanApplication &operator=(const VulkanApplication &) = delete;

  // Frame-Level methods
  virtual void mainLoop();
  virtual void updateUniformBuffer(uint32_t currentImage);
  virtual void drawFrame();
  void recreateSwapchain();

  // Application Lifecycle methods
  void run() {
    configure();
    initVulkan();
    setting();

    mainLoop();

    cleanup();
  }

  VulkanContext ctx;

  std::unique_ptr<Window> window;
  std::unique_ptr<Camera> camera;
  std::unique_ptr<Instance> instance;
  std::unique_ptr<Surface> surface;
  std::unique_ptr<Device> device;

  std::unique_ptr<Swapchain> swapchain;
  std::unique_ptr<Framebuffers> swapchainFramebuffers;

  std::unique_ptr<RenderPass> renderPass;
  std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
  std::unique_ptr<GraphicsPipeline> graphicsPipeline;

  std::unique_ptr<CommandPool> commandPool;
  std::unique_ptr<VertexBuffer> vertexBuffer;
  std::unique_ptr<IndexBuffer> indexBuffer;
  std::unique_ptr<UniformBuffers> uniformBuffers;
  std::unique_ptr<DescriptorSet> descriptorSet;
  std::unique_ptr<CommandBuffers> commandBuffers;

  std::unique_ptr<SyncObjects> syncObjects;

private:
  // Initialization methods
  virtual void configure();
  virtual void setting();

  void initVulkan();
  void cleanup();
};

static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  auto app =
      reinterpret_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));
  app->ctx.framebufferResized = true;
}
