#pragma once

#include "vkr/context.hh"
#include "vkr/logger.hh"
#include "vkr/render/graph.hh"
#include "vkr/render/passes/raster.hh"
#include "vkr/render/passes/ui.hh"
#include "vkr/render/renderer.hh"
#include "vkr/resource/manager.hh"
#include "vkr/scene/camera.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/timer.hh"
#include "vkr/util/toml.hh"
#include <filesystem>
#include <memory>
#include <vector>

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
  std::unique_ptr<util::AssetSystem> assetSystem;

  std::unique_ptr<core::Window> window;
  std::unique_ptr<core::Instance> instance;
  std::unique_ptr<core::Surface> surface;
  std::unique_ptr<core::Device> device;
  std::unique_ptr<core::Swapchain> swapchain;
  std::unique_ptr<core::CommandPool> commandPool;
  std::unique_ptr<core::SyncObjects> syncObjects;

  // resource management
  std::unique_ptr<resource::ResourceManager> resourceManager;

  // renderer
  std::unique_ptr<render::Renderer> renderer;
  std::unique_ptr<render::RenderGraph> renderGraph;
  render::RasterPass *rasterPass{nullptr};
  render::UiPass *uiPass{nullptr};

  // components
  std::unique_ptr<scene::Camera> camera;
  std::unique_ptr<util::Timer> timer;

protected:
  virtual void onConfigure() {}
  virtual void onDrawFrame(uint32_t currentImage) {}
  virtual void createResources() {}
  virtual void buildRenderGraph(render::RenderGraph &graph);

  [[nodiscard]] auto makeDefaultRasterPassDesc() const
      -> render::RasterPassDesc;

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

} // namespace vkr
