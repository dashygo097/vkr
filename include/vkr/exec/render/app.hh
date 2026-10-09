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
#include "vkr/logger.hh"
#include "vkr/scene/camera.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/ui.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/input_tracer.hh"
#include "vkr/util/timer.hh"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace vkr::exec {

class UiPass;

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

  [[nodiscard]] static auto
  windowed(std::string appName, std::string windowTitle, uint32_t width = 1200,
           uint32_t height = 900, uint32_t framesInFlight = 2)
      -> RenderAppDesc {
    RenderAppDesc desc{};
    desc.window = {
        .title = std::move(windowTitle),
        .width = width,
        .height = height,
    };
    desc.instance = {
        .name = std::move(appName),
        .version = VK_MAKE_VERSION(1, 0, 0),
        .surfaceIntegration = core::SurfaceIntegration::GLFW,
    };
    desc.commandBuffers.size = framesInFlight;
    return desc;
  }

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

protected:
  RenderAppDesc ctx{};

  [[nodiscard]] auto assetSystem() -> util::AssetSystem & {
    if (!asset_system_) {
      VKR_EXEC_ERROR(
          "RenderApplication::assetSystem() requested before initialization");
    }
    return *asset_system_;
  }

  [[nodiscard]] auto assetSystem() const -> const util::AssetSystem & {
    if (!asset_system_) {
      VKR_EXEC_ERROR(
          "RenderApplication::assetSystem() requested before initialization");
    }
    return *asset_system_;
  }

  [[nodiscard]] auto window() -> core::Window & {
    if (!window_) {
      VKR_EXEC_ERROR(
          "RenderApplication::window() requested before initialization");
    }
    return *window_;
  }

  [[nodiscard]] auto window() const -> const core::Window & {
    if (!window_) {
      VKR_EXEC_ERROR(
          "RenderApplication::window() requested before initialization");
    }
    return *window_;
  }

  [[nodiscard]] auto instance() -> core::Instance & {
    if (!instance_) {
      VKR_EXEC_ERROR(
          "RenderApplication::instance() requested before initialization");
    }
    return *instance_;
  }

  [[nodiscard]] auto instance() const -> const core::Instance & {
    if (!instance_) {
      VKR_EXEC_ERROR(
          "RenderApplication::instance() requested before initialization");
    }
    return *instance_;
  }

  [[nodiscard]] auto surface() -> core::Surface & {
    if (!surface_) {
      VKR_EXEC_ERROR(
          "RenderApplication::surface() requested before initialization");
    }
    return *surface_;
  }

  [[nodiscard]] auto surface() const -> const core::Surface & {
    if (!surface_) {
      VKR_EXEC_ERROR(
          "RenderApplication::surface() requested before initialization");
    }
    return *surface_;
  }

  [[nodiscard]] auto device() -> core::Device & {
    if (!device_) {
      VKR_EXEC_ERROR(
          "RenderApplication::device() requested before initialization");
    }
    return *device_;
  }

  [[nodiscard]] auto device() const -> const core::Device & {
    if (!device_) {
      VKR_EXEC_ERROR(
          "RenderApplication::device() requested before initialization");
    }
    return *device_;
  }

  [[nodiscard]] auto swapchain() -> core::Swapchain & {
    if (!swapchain_) {
      VKR_EXEC_ERROR(
          "RenderApplication::swapchain() requested before initialization");
    }
    return *swapchain_;
  }

  [[nodiscard]] auto swapchain() const -> const core::Swapchain & {
    if (!swapchain_) {
      VKR_EXEC_ERROR(
          "RenderApplication::swapchain() requested before initialization");
    }
    return *swapchain_;
  }

  [[nodiscard]] auto commandPool() -> core::CommandPool & {
    if (!command_pool_) {
      VKR_EXEC_ERROR(
          "RenderApplication::commandPool() requested before initialization");
    }
    return *command_pool_;
  }

  [[nodiscard]] auto commandPool() const -> const core::CommandPool & {
    if (!command_pool_) {
      VKR_EXEC_ERROR(
          "RenderApplication::commandPool() requested before initialization");
    }
    return *command_pool_;
  }

  [[nodiscard]] auto commandBuffers() -> core::CommandBuffers & {
    if (!command_buffers_) {
      VKR_EXEC_ERROR("RenderApplication::commandBuffers() requested before "
                     "initialization");
    }
    return *command_buffers_;
  }

  [[nodiscard]] auto commandBuffers() const -> const core::CommandBuffers & {
    if (!command_buffers_) {
      VKR_EXEC_ERROR("RenderApplication::commandBuffers() requested before "
                     "initialization");
    }
    return *command_buffers_;
  }

  [[nodiscard]] auto scene() -> vkr::scene::Scene & {
    if (!scene_) {
      VKR_EXEC_ERROR(
          "RenderApplication::scene() requested before initialization");
    }
    return *scene_;
  }

  [[nodiscard]] auto scene() const -> const vkr::scene::Scene & {
    if (!scene_) {
      VKR_EXEC_ERROR(
          "RenderApplication::scene() requested before initialization");
    }
    return *scene_;
  }

  [[nodiscard]] auto inputTracer() -> util::InputTracer & {
    if (!input_tracer_) {
      VKR_EXEC_ERROR(
          "RenderApplication::inputTracer() requested before initialization");
    }
    return *input_tracer_;
  }

  [[nodiscard]] auto inputTracer() const -> const util::InputTracer & {
    if (!input_tracer_) {
      VKR_EXEC_ERROR(
          "RenderApplication::inputTracer() requested before initialization");
    }
    return *input_tracer_;
  }

  [[nodiscard]] auto executor() -> RenderExecutor & {
    if (!executor_) {
      VKR_EXEC_ERROR(
          "RenderApplication::executor() requested before initialization");
    }
    return *executor_;
  }

  [[nodiscard]] auto executor() const -> const RenderExecutor & {
    if (!executor_) {
      VKR_EXEC_ERROR(
          "RenderApplication::executor() requested before initialization");
    }
    return *executor_;
  }

  [[nodiscard]] auto graph() -> RenderGraph & {
    if (!graph_) {
      VKR_EXEC_ERROR(
          "RenderApplication::graph() requested before initialization");
    }
    return *graph_;
  }

  [[nodiscard]] auto graph() const -> const RenderGraph & {
    if (!graph_) {
      VKR_EXEC_ERROR(
          "RenderApplication::graph() requested before initialization");
    }
    return *graph_;
  }

  [[nodiscard]] auto profiler() -> Profiler & {
    if (!profiler_) {
      VKR_EXEC_ERROR(
          "RenderApplication::profiler() requested before initialization");
    }
    return *profiler_;
  }

  [[nodiscard]] auto profiler() const -> const Profiler & {
    if (!profiler_) {
      VKR_EXEC_ERROR(
          "RenderApplication::profiler() requested before initialization");
    }
    return *profiler_;
  }

  [[nodiscard]] auto camera() -> vkr::scene::Camera & {
    if (!camera_) {
      VKR_EXEC_ERROR(
          "RenderApplication::camera() requested before initialization");
    }
    return *camera_;
  }

  [[nodiscard]] auto camera() const -> const vkr::scene::Camera & {
    if (!camera_) {
      VKR_EXEC_ERROR(
          "RenderApplication::camera() requested before initialization");
    }
    return *camera_;
  }

  [[nodiscard]] auto timer() -> util::Timer & {
    if (!timer_) {
      VKR_EXEC_ERROR(
          "RenderApplication::timer() requested before initialization");
    }
    return *timer_;
  }

  [[nodiscard]] auto timer() const -> const util::Timer & {
    if (!timer_) {
      VKR_EXEC_ERROR(
          "RenderApplication::timer() requested before initialization");
    }
    return *timer_;
  }

  [[nodiscard]] auto ui() -> vkr::ui::UI & {
    if (!ui_) {
      VKR_EXEC_ERROR("UI requested before initialization");
    }
    return ui_->get();
  }

  [[nodiscard]] auto ui() const -> const vkr::ui::UI & {
    if (!ui_) {
      VKR_EXEC_ERROR("UI requested before initialization");
    }
    return ui_->get();
  }

  [[nodiscard]] auto resolve(std::string_view path) const
      -> std::filesystem::path {
    return assetSystem().resolve(path);
  }

  virtual void configure() {}
  virtual void onDraw() {}
  virtual void createResources() {}
  virtual void buildGraph() = 0;
  [[nodiscard]] virtual auto shouldClose() const -> bool { return false; }

  [[nodiscard]] virtual auto snapshotPath() const -> std::filesystem::path {
    return "snapshot.toml";
  }

private:
  // components
  std::unique_ptr<util::AssetSystem> asset_system_{};
  std::unique_ptr<core::Window> window_{};
  std::unique_ptr<core::Instance> instance_{};
  std::unique_ptr<core::Surface> surface_{};
  std::unique_ptr<core::Device> device_{};
  std::unique_ptr<core::Swapchain> swapchain_{};
  std::unique_ptr<core::CommandPool> command_pool_{};
  std::unique_ptr<core::CommandBuffers> command_buffers_{};
  std::unique_ptr<vkr::scene::Scene> scene_{};
  std::unique_ptr<util::InputTracer> input_tracer_{};
  std::unique_ptr<RenderExecutor> executor_{};
  std::unique_ptr<RenderGraph> graph_{};
  std::unique_ptr<Profiler> profiler_{};
  std::unique_ptr<vkr::scene::Camera> camera_{};
  std::unique_ptr<util::Timer> timer_{};
  std::optional<std::reference_wrapper<vkr::ui::UI>> ui_{};

  // helpers
  void initVulkan();

  void mainLoop();
  void drawFrame();
  auto buildPresentation() -> UiPass &;
  void recreateSwapchain();

  void loadSnapshot();
  void saveSnapshot();
};

} // namespace vkr::exec
