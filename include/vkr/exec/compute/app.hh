#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/exec/compute/executor.hh"
#include "vkr/exec/compute/graph.hh"
#include "vkr/exec/profiler.hh"
#include "vkr/logger.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/timer.hh"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace vkr::exec {

struct ComputeAppDesc {
  util::AssetDesc asset{};
  core::InstanceDesc instance{};
  core::DeviceDesc device{};
  core::CommandPoolDesc commandPool{core::CommandQueueRole::Compute};
  ProfilerDesc profiler{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return asset.isValid() && instance.isValid() && device.isValid() &&
           commandPool.isValid() && profiler.isValid();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("asset", asset);
    ar("instance", instance);
    ar("device", device);
    ar("commandPool", commandPool);
    ar("profiler", profiler);
  }
};

class ComputeApplication {
public:
  ComputeApplication() = default;
  virtual ~ComputeApplication() = default;

  ComputeApplication(const ComputeApplication &) = delete;
  auto operator=(const ComputeApplication &) -> ComputeApplication & = delete;

  void run();
  void benchmark(uint32_t warmupRuns, uint32_t measuredRuns);

protected:
  ComputeAppDesc ctx{};

  [[nodiscard]] auto assetSystem() -> util::AssetSystem & {
    if (!asset_system_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::assetSystem() requested before initialization");
    }
    return *asset_system_;
  }

  [[nodiscard]] auto assetSystem() const -> const util::AssetSystem & {
    if (!asset_system_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::assetSystem() requested before initialization");
    }
    return *asset_system_;
  }

  [[nodiscard]] auto timer() -> util::Timer & {
    if (!timer_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::timer() requested before initialization");
    }
    return *timer_;
  }

  [[nodiscard]] auto timer() const -> const util::Timer & {
    if (!timer_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::timer() requested before initialization");
    }
    return *timer_;
  }

  [[nodiscard]] auto instance() -> core::Instance & {
    if (!instance_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::instance() requested before initialization");
    }
    return *instance_;
  }

  [[nodiscard]] auto instance() const -> const core::Instance & {
    if (!instance_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::instance() requested before initialization");
    }
    return *instance_;
  }

  [[nodiscard]] auto device() -> core::Device & {
    if (!device_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::device() requested before initialization");
    }
    return *device_;
  }

  [[nodiscard]] auto device() const -> const core::Device & {
    if (!device_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::device() requested before initialization");
    }
    return *device_;
  }

  [[nodiscard]] auto commandPool() -> core::CommandPool & {
    if (!command_pool_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::commandPool() requested before initialization");
    }
    return *command_pool_;
  }

  [[nodiscard]] auto commandPool() const -> const core::CommandPool & {
    if (!command_pool_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::commandPool() requested before initialization");
    }
    return *command_pool_;
  }

  [[nodiscard]] auto executor() -> ComputeExecutor & {
    if (!executor_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::executor() requested before initialization");
    }
    return *executor_;
  }

  [[nodiscard]] auto executor() const -> const ComputeExecutor & {
    if (!executor_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::executor() requested before initialization");
    }
    return *executor_;
  }

  [[nodiscard]] auto graph() -> ComputeGraph & {
    if (!graph_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::graph() requested before initialization");
    }
    return *graph_;
  }

  [[nodiscard]] auto graph() const -> const ComputeGraph & {
    if (!graph_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::graph() requested before initialization");
    }
    return *graph_;
  }

  [[nodiscard]] auto profiler() -> Profiler & {
    if (!profiler_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::profiler() requested before initialization");
    }
    return *profiler_;
  }

  [[nodiscard]] auto profiler() const -> const Profiler & {
    if (!profiler_) {
      VKR_EXEC_ERROR(
          "ComputeApplication::profiler() requested before initialization");
    }
    return *profiler_;
  }

  [[nodiscard]] auto resolve(std::string_view path) const
      -> std::filesystem::path {
    return assetSystem().resolve(path);
  }

  virtual void configure() {}
  virtual void createResources() {}
  virtual void buildGraph() = 0;
  virtual void afterExecute(const ProfileReport &) {}

private:
  // components
  std::unique_ptr<util::AssetSystem> asset_system_{};
  std::unique_ptr<util::Timer> timer_{};
  std::unique_ptr<core::Instance> instance_{};
  std::unique_ptr<core::Device> device_{};
  std::unique_ptr<core::CommandPool> command_pool_{};
  std::unique_ptr<ComputeExecutor> executor_{};
  std::unique_ptr<ComputeGraph> graph_{};
  std::unique_ptr<Profiler> profiler_{};

  // helpers
  void initCompute();
  [[nodiscard]] auto execute(bool capture) -> ProfileReport;
};

} // namespace vkr::exec
