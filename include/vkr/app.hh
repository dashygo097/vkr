#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/sync/objects.hh"
#include "vkr/core/window.hh"
#include "vkr/logger.hh"
#include "vkr/render/graph.hh"
#include "vkr/render/passes/composite.hh"
#include "vkr/render/passes/fullscreen.hh"
#include "vkr/render/passes/post_process.hh"
#include "vkr/render/passes/raster.hh"
#include "vkr/render/passes/skybox.hh"
#include "vkr/render/passes/ui.hh"
#include "vkr/render/renderer.hh"
#include "vkr/resource/manager.hh"
#include "vkr/scene/camera.hh"
#include "vkr/ui/ui.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/input_tracer.hh"
#include "vkr/util/timer.hh"
#include "vkr/util/toml.hh"
#include <filesystem>
#include <memory>
#include <vector>

namespace vkr {

struct AppDesc {
  util::AssetDesc asset{};
  core::WindowDesc window{};
  core::InstanceDesc instance{};
  core::DeviceDesc device{};
  core::SwapchainDesc swapchain{};
  core::CommandPoolDesc graphicsCommandPool{};
  core::CommandPoolDesc computeCommandPool{core::CommandQueueRole::Compute};
  core::CommandPoolDesc transferCommandPool{core::CommandQueueRole::Transfer};
  scene::CameraDesc camera{};
  ui::UiDesc ui{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return asset.isValid() && window.isValid() && instance.isValid() &&
           device.isValid() && swapchain.isValid() &&
           graphicsCommandPool.isValid() && computeCommandPool.isValid() &&
           transferCommandPool.isValid() && camera.isValid() && ui.isValid();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("asset", asset);
    ar("window", window);
    ar("instance", instance);
    ar("device", device);
    ar("swapchain", swapchain);
    ar("graphicsCommandPool", graphicsCommandPool);
    ar("computeCommandPool", computeCommandPool);
    ar("transferCommandPool", transferCommandPool);
    ar("camera", camera);
    ar("ui", ui);
  }
};

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

  AppDesc ctx;

  // core
  std::unique_ptr<util::AssetSystem> assetSystem;

  std::unique_ptr<core::Window> window;
  std::unique_ptr<core::Instance> instance;
  std::unique_ptr<core::Surface> surface;
  std::unique_ptr<core::Device> device;
  std::unique_ptr<core::Swapchain> swapchain;
  std::unique_ptr<core::CommandPool> graphicsCommandPool;
  std::unique_ptr<core::CommandPool> computeCommandPool;
  std::unique_ptr<core::CommandPool> transferCommandPool;
  std::unique_ptr<core::SyncObjects> syncObjects;

  // resource management
  std::unique_ptr<resource::ResourceManager> resourceManager;

  // input
  std::unique_ptr<util::InputTracer> inputTracer;

  // renderer
  std::unique_ptr<render::Renderer> renderer;
  std::unique_ptr<render::RenderGraph> renderGraph;

  // components
  std::unique_ptr<scene::Camera> camera;
  std::unique_ptr<util::Timer> timer;

protected:
  virtual void configure() {}
  virtual void onDraw() {}
  virtual void createResources() {}
  virtual void buildRenderGraph() = 0;
  [[nodiscard]] virtual auto shouldClose() const -> bool;

  [[nodiscard]] virtual auto snapshotPath() const -> std::filesystem::path {
    return "snapshot.toml";
  }

private:
  void initVulkan();

  void mainLoop();
  void drawFrame();
  void updateUiState();
  void recreateSwapchain();

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

} // namespace vkr
