#pragma once

#include "./core/core.hh"
#include "./ctx.hh"
#include "./pipeline/graphics_pipeline.hh"
#include "./rendering/renderer.hh"
#include "./resources/manager.hh"
#include "./scene/scene.hh"
#include "./timer.hh"
#include "./ui/ui.hh"

namespace vkr {
class VulkanApplication {
public:
  VulkanApplication() = default;
  virtual ~VulkanApplication() { cleanup(); }

  VulkanApplication(const VulkanApplication &) = delete;
  VulkanApplication &operator=(const VulkanApplication &) = delete;

  void run() {
    onConfigure();
    initVulkan();
    onSetup();

    mainLoop();

    cleanup();
  }

  VulkanContext ctx;

  // core
  std::unique_ptr<core::Window> window;
  std::unique_ptr<core::Instance> instance;
  std::unique_ptr<core::Surface> surface;
  std::unique_ptr<core::Device> device;
  std::unique_ptr<core::Swapchain> swapchain;
  std::unique_ptr<core::CommandPool> commandPool;
  std::unique_ptr<core::CommandBuffers> commandBuffers;
  std::unique_ptr<core::SyncObjects> syncObjects;

  // resource management
  std::unique_ptr<resource::ResourceManager> resourceManager;

  // pipeline
  std::unique_ptr<pipeline::RenderPass> renderPass;
  std::unique_ptr<pipeline::GraphicsPipeline> graphicsPipeline;

  // descriptor
  std::unique_ptr<pipeline::DescriptorManager> descriptorManager;
  std::shared_ptr<pipeline::DescriptorSetLayout> descriptorSetLayout;
  std::unique_ptr<pipeline::DescriptorPool> descriptorPool;
  std::unique_ptr<pipeline::DescriptorSets> descriptorSets;

  // renderer
  std::unique_ptr<render::Renderer> renderer;

  // components
  std::unique_ptr<scene::Camera> camera;
  std::unique_ptr<Timer> timer;
  std::unique_ptr<ui::UI> ui;

protected:
  virtual void onConfigure() {}
  virtual void onSetup() {}
  virtual void onUpdate(float deltaTime) {}
  virtual void onCleanup() {}

  virtual void createUniforms() {}
  virtual std::vector<pipeline::DescriptorBinding> createDescriptorBindings() {
    return {};
  }
  virtual void bindDescriptorSets() {}
  virtual void updateUniforms(uint32_t frameIndex) {}

private:
  // Initialization methods
  void initVulkan();

  // Frame-Level methods
  void mainLoop();
  void recreateSwapchain();
  void drawFrame();

  // Cleanup method
  void cleanup();
};

static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  auto app =
      reinterpret_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));
  app->ctx.framebufferResized = true;
}

} // namespace vkr
