#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/window.hh"
#include "vkr/exec/profiler.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/graph.hh"
#include "vkr/exec/render/sync.hh"
#include "vkr/scene/camera.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/ui.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/input_tracer.hh"
#include "vkr/util/timer.hh"
#include <filesystem>
#include <memory>

namespace vkr::exec {

struct RenderAppDesc {
  util::AssetDesc asset{};
  core::WindowDesc window{};
  core::InstanceDesc instance{};
  core::DeviceDesc device{};
  core::SwapchainDesc swapchain{};
  core::CommandPoolDesc commandPool{};
  core::CommandBuffersDesc commandBuffers{};
  ProfilerDesc profiler{};
  vkr::scene::CameraDesc camera{};
  ui::UiDesc ui{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return asset.isValid() && window.isValid() && instance.isValid() &&
           device.isValid() && swapchain.isValid() && commandPool.isValid() &&
           commandBuffers.isValid() && profiler.isValid() && camera.isValid() &&
           ui.isValid();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("asset", asset);
    ar("window", window);
    ar("instance", instance);
    ar("device", device);
    ar("swapchain", swapchain);
    ar("commandPool", commandPool);
    ar("commandBuffers", commandBuffers);
    ar("profiler", profiler);
    ar("camera", camera);
    ar("ui", ui);
  }
};

class RenderApplication {
public:
  RenderApplication() = default;
  virtual ~RenderApplication() = default;

  RenderApplication(const RenderApplication &) = delete;
  auto operator=(const RenderApplication &) -> RenderApplication & = delete;

  void run();

  RenderAppDesc ctx;

  // core
  std::unique_ptr<util::AssetSystem> assetSystem;

  std::unique_ptr<core::Window> window;
  std::unique_ptr<core::Instance> instance;
  std::unique_ptr<core::Surface> surface;
  std::unique_ptr<core::Device> device;
  std::unique_ptr<core::Swapchain> swapchain;
  std::unique_ptr<core::CommandPool> commandPool;
  std::unique_ptr<core::CommandBuffers> commandBuffers;
  std::unique_ptr<FrameSync> frameSync;

  // resource management
  std::unique_ptr<vkr::scene::Scene> scene;

  // input
  std::unique_ptr<util::InputTracer> inputTracer;

  // executor
  std::unique_ptr<Executor> executor;
  std::unique_ptr<RenderGraph> graph;
  std::unique_ptr<Profiler> profiler;
  ProfileReport profileReport;

  // components
  std::unique_ptr<vkr::scene::Camera> camera;
  std::unique_ptr<util::Timer> timer;

protected:
  virtual void configure() {}
  virtual void onDraw() {}
  virtual void createResources() {}
  virtual void buildGraph() = 0;
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

  void loadSnapshot();
  void saveSnapshot();
};

} // namespace vkr::exec
