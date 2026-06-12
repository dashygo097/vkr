#pragma once

#include "vkr/context.hh"
#include "vkr/core/core.hh"
#include "vkr/logger.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/render/renderer.hh"
#include "vkr/resource/manager.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/ui.hh"
#include "vkr/util/timer.hh"
#include "vkr/util/toml.hh"

#include <filesystem>

namespace vkr {

class VulkanApplication {
public:
  VulkanApplication() = default;
  virtual ~VulkanApplication() = default;

  VulkanApplication(const VulkanApplication &) = delete;
  auto operator=(const VulkanApplication &) -> VulkanApplication & = delete;

  void run() {
    initVulkan();

    try {
      mainLoop();
      saveSnapshot();
    } catch (...) {
      saveSnapshot();
      throw;
    }
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
  std::shared_ptr<pipeline::DescriptorSetLayout> descriptorSetLayout;
  std::unique_ptr<pipeline::DescriptorPool> descriptorPool;
  std::unique_ptr<pipeline::DescriptorSets> descriptorSets;

  // ui
  std::unique_ptr<ui::UI> ui;

  // renderer
  std::unique_ptr<render::Renderer> renderer;

  // components
  std::unique_ptr<scene::Camera> camera;
  std::unique_ptr<util::Timer> timer;

protected:
  virtual void onConfigure() {}
  virtual void onDrawFrame(uint32_t currentImage) {}

  virtual void createResources() {}
  virtual auto createDescriptorBindings()
      -> std::vector<pipeline::DescriptorBinding> {
    return {};
  }

  [[nodiscard]] virtual auto snapshotPath() const -> std::filesystem::path {
    return "snapshot.toml";
  }

private:
  void initVulkan();

  void mainLoop();
  void drawFrame();

  void loadSnapshot() {
    const auto path = snapshotPath();

    if (!std::filesystem::exists(path)) {
      VKR_UTIL_INFO("snapshot not found, using default config: {}",
                    path.string());
      return;
    }

    if (!vkr::util::loadTomlFile(path, ctx)) {
      VKR_UTIL_WARN("failed to load snapshot, using default config: {}",
                    path.string());
    }
  }

  void saveSnapshot() {
    const auto path = snapshotPath();

    if (!vkr::util::saveTomlFile(path, ctx)) {
      VKR_UTIL_WARN("failed to save snapshot: {}", path.string());
    }
  }
};

static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  auto *app =
      reinterpret_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));
  if (!app) {
    return;
  }

  app->ctx.framebufferResized = true;
}

} // namespace vkr
