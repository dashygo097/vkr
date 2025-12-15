#pragma once

#include "./components/camera.hh"
#include "./components/fps_counter.hh"
#include "./components/ui/ui.hh"
#include "./core/core.hh"
#include "./ctx.hh"
#include "./pipeline/pipeline.hh"
#include "./resources/manager.hh"

namespace vkr {
class VulkanApplication {
public:
  VulkanApplication() = default;
  ~VulkanApplication() { cleanup(); }

  VulkanApplication(const VulkanApplication &) = delete;
  VulkanApplication &operator=(const VulkanApplication &) = delete;

  // Application Lifecycle methods
  void run() {
    configure();
    initVulkan();
    setting();

    mainLoop();

    cleanup();
  }

  VulkanContext ctx;

  // core
  std::unique_ptr<Window> window;
  std::unique_ptr<Instance> instance;
  std::unique_ptr<Surface> surface;
  std::unique_ptr<Device> device;
  std::unique_ptr<Swapchain> swapchain;
  std::unique_ptr<CommandPool> commandPool;
  std::unique_ptr<CommandBuffers> commandBuffers;
  std::unique_ptr<SyncObjects> syncObjects;

  // resource management
  std::unique_ptr<ResourceManager> resourceManager;

  // pipeline
  std::unique_ptr<RenderPass> renderPass;
  std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
  std::unique_ptr<GraphicsPipeline> graphicsPipeline;
  std::unique_ptr<DescriptorSets> descriptorSets;

  // components
  std::unique_ptr<Camera> camera;
  std::unique_ptr<FPSCounter> fpsCounter;
  std::unique_ptr<UI> ui;

private:
  // Initialization methods
  virtual void configure();
  virtual void setting();

  // Frame-Level methods
  virtual void mainLoop();
  virtual void updateUniformBuffer(uint32_t currentImage);
  virtual void drawFrame();

  void recreateSwapchain();
  void initVulkan();
  void cleanup();
}; // namespace vkr

static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  auto app =
      reinterpret_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));
  app->ctx.framebufferResized = true;
};
} // namespace vkr
