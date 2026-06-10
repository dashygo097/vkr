#pragma once

#include "vkr/core/core.hh"
#include "vkr/ctx.hh"
#include "vkr/logger.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/rendering/renderer.hh"
#include "vkr/resources/manager.hh"
#include "vkr/scene/scene.hh"
#include "vkr/timer.hh"
#include "vkr/ui/ui.hh"

namespace vkr {
class VulkanApplication {
public:
  VulkanApplication() = default;
  virtual ~VulkanApplication() = default;

  VulkanApplication(const VulkanApplication &) = delete;
  auto operator=(const VulkanApplication &) -> VulkanApplication & = delete;

  void run() {
    initVulkan();
    mainLoop();
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
  std::unique_ptr<resource::DepthResources> depthResources;
  std::unique_ptr<resource::ResourceManager> resourceManager;
  std::unique_ptr<resource::OffscreenTarget> offscreenTarget;

  // pipeline
  std::unique_ptr<pipeline::RenderPass> swapchainRenderPass;
  std::unique_ptr<pipeline::RenderPass> offscreenRenderPass;
  std::unique_ptr<pipeline::GraphicsPipeline> graphicsPipeline;

  // descriptor
  std::unique_ptr<pipeline::DescriptorManager> descriptorManager;
  std::shared_ptr<pipeline::DescriptorSetLayout> descriptorSetLayout;
  std::unique_ptr<pipeline::DescriptorPool> descriptorPool;
  std::unique_ptr<pipeline::DescriptorSets> descriptorSets;

  // ui
  std::unique_ptr<ui::UI> ui;

  // renderer
  std::unique_ptr<render::Renderer> renderer;

  // components
  std::unique_ptr<scene::Camera> camera;
  std::unique_ptr<Timer> timer;

protected:
  virtual void onConfigure() {}
  virtual void onSetup() {}
  virtual void onDrawFrame(uint32_t currentImage) {}

  virtual void createUniforms() {}
  virtual auto createDescriptorBindings()
      -> std::vector<pipeline::DescriptorBinding> {
    return {};
  }
  virtual void bindDescriptorSets() {}

private:
  // Initialization methods
  void initVulkan();

  // Frame-Level methods
  void mainLoop();
  void recreateSwapchain();
  void drawFrame();
};

static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  auto app =
      reinterpret_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));
  app->ctx.framebufferResized = true;
}

} // namespace vkr
