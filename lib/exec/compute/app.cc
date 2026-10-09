#include "vkr/exec/compute/app.hh"
#include <vector>

namespace vkr::exec {

void ComputeApplication::run() {
  initCompute();
  try {
    const auto report = execute(true);
    if (ctx.profiler.logReport) {
      report.log();
    }
    afterExecute(report);
  } catch (...) {
    device_->waitIdle();
    throw;
  }
}

void ComputeApplication::benchmark(uint32_t warmupRuns, uint32_t measuredRuns) {
  if (measuredRuns == 0) {
    VKR_EXEC_ERROR("Compute benchmark requires at least one measured run");
  }

  initCompute();
  try {
    for (uint32_t run = 0; run < warmupRuns; ++run) {
      (void)execute(false);
    }

    std::vector<ProfileReport> captures{};
    captures.reserve(measuredRuns);
    for (uint32_t run = 0; run < measuredRuns; ++run) {
      captures.push_back(execute(true));
    }

    const auto report = ProfileReport::aggregate(captures);
    if (ctx.profiler.logReport) {
      report.log();
    }
    afterExecute(report);
  } catch (...) {
    device_->waitIdle();
    throw;
  }
}

void ComputeApplication::initCompute() {
  if (instance_) {
    VKR_EXEC_ERROR("ComputeApplication has already been initialized");
  }

  Logger::init();
  configure();
  ctx.instance.surfaceIntegration = core::SurfaceIntegration::None;
  ctx.commandPool.queueRole = core::CommandQueueRole::Compute;

  if (!ctx.isValid()) {
    VKR_CORE_ERROR("invalid compute app config");
  }

  asset_system_ = std::make_unique<util::AssetSystem>(ctx.asset);
  timer_ = std::make_unique<util::Timer>();

  instance_ = std::make_unique<core::Instance>(ctx.instance);
  device_ = std::make_unique<core::Device>(*instance_, ctx.device);
  if (!device_->supportsCompute()) {
    VKR_CORE_ERROR("compute application requires compute queue support");
  }

  command_pool_ = std::make_unique<core::CommandPool>(*device_, ctx.commandPool);
  profiler_ = std::make_unique<Profiler>(*device_, *command_pool_, ctx.profiler);
  executor_ = std::make_unique<ComputeExecutor>(*device_, *command_pool_);

  createResources();

  graph_ = std::make_unique<ComputeGraph>();
  buildGraph();
  graph_->compile();
  graph_->create();
}

auto ComputeApplication::execute(bool capture) -> ProfileReport {
  if (capture) {
    executor_->setProfiler(*profiler_);
  } else {
    executor_->clearProfiler();
  }

  executor_->begin();
  if (capture) {
    timer_->reset();
  }
  executor_->beginProfileScope("compute_graph");
  graph_->record();
  executor_->endProfileScope();

  double recordMs = 0.0;
  if (capture) {
    timer_->update();
    recordMs = timer_->elapsedMilliseconds();
    timer_->reset();
  }

  executor_->submitAndWait();
  double submitWaitMs = 0.0;
  if (capture) {
    timer_->update();
    submitWaitMs = timer_->elapsedMilliseconds();
  }
  executor_->end();

  if (!capture) {
    return {};
  }

  auto report = profiler_->collect();
  report.cpuSamples.push_back(ProfileSample{
      .name = "compute_graph.record",
      .milliseconds = recordMs,
      .minMilliseconds = recordMs,
      .medianMilliseconds = recordMs,
      .maxMilliseconds = recordMs,
  });
  report.cpuSamples.push_back(ProfileSample{
      .name = "compute_graph.submit_wait",
      .milliseconds = submitWaitMs,
      .minMilliseconds = submitWaitMs,
      .medianMilliseconds = submitWaitMs,
      .maxMilliseconds = submitWaitMs,
  });
  return report;
}

} // namespace vkr::exec
