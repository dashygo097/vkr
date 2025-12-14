#pragma once

#include "./components/camera.hh"
#include "./components/fps_counter.hh"
#include "./components/ui/ui.hh"
#include "./core/core.hh"
#include "./ctx.hh"
#include "./pipeline/pipeline.hh"
#include "./resources/resources.hh"

namespace vkr {
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
  std::unique_ptr<std::vector<std::shared_ptr<VertexBuffer>>> vertexBuffers;
  std::unique_ptr<std::vector<std::shared_ptr<IndexBuffer>>> indexBuffers;
  std::unique_ptr<UniformBuffers> uniformBuffers;
  std::unique_ptr<DescriptorSets> descriptorSets;
  std::unique_ptr<CommandBuffers> commandBuffers;

  std::unique_ptr<SyncObjects> syncObjects;

  std::unique_ptr<FPSCounter> fpsCounter;
  std::unique_ptr<UI> ui;

private:
  // Initialization methods
  virtual void configure();
  virtual void setting();

  void recreateSwapchain();
  void initVulkan();
  void cleanup();
};

static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  auto app =
      reinterpret_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));
  app->ctx.framebufferResized = true;
};
} // namespace vkr
