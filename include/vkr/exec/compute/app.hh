#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/exec/compute/executor.hh"
#include "vkr/exec/compute/graph.hh"
#include "vkr/exec/profiler/profiler.hh"
#include "vkr/logger.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/timer.hh"
#include <memory>

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

  ComputeAppDesc ctx;

  std::unique_ptr<util::AssetSystem> assetSystem;
  std::unique_ptr<util::Timer> timer;
  std::unique_ptr<core::Instance> instance;
  std::unique_ptr<core::Device> device;
  std::unique_ptr<core::CommandPool> commandPool;
  std::unique_ptr<Profiler> profiler;
  std::unique_ptr<ComputeExecutor> executor;
  std::unique_ptr<ComputeGraph> graph;
  ProfileReport profileReport;

protected:
  virtual void configure() {}
  virtual void createResources() {}
  virtual void buildGraph() = 0;
  virtual void afterExecute() {}

private:
  void initCompute();
  void execute();
};

} // namespace vkr::exec
