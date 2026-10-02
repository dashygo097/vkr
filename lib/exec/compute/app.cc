#include "vkr/exec/compute/app.hh"
#include <vector>

namespace vkr::exec {

void ComputeApplication::run() {
  initCompute();
  try {
    profileReport = execute(true);
    if (ctx.profiler.logReport) {
      profileReport.log();
    }
    afterExecute();
  } catch (...) {
    device->waitIdle();
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

    profileReport = ProfileReport::aggregate(captures);
    if (ctx.profiler.logReport) {
      profileReport.log();
    }
    afterExecute();
  } catch (...) {
    device->waitIdle();
    throw;
  }
}

void ComputeApplication::initCompute() {
  if (instance) {
    VKR_EXEC_ERROR("ComputeApplication has already been initialized");
  }

  Logger::init();
  configure();
  ctx.instance.surfaceIntegration = core::SurfaceIntegration::None;
  ctx.commandPool.queueRole = core::CommandQueueRole::Compute;

  if (!ctx.isValid()) {
    VKR_CORE_ERROR("invalid compute app config");
  }

  assetSystem = std::make_unique<util::AssetSystem>(ctx.asset);
  timer = std::make_unique<util::Timer>();

  instance = std::make_unique<core::Instance>(ctx.instance);
  device = std::make_unique<core::Device>(*instance, ctx.device);
  if (!device->supportsCompute()) {
    VKR_CORE_ERROR("compute application requires compute queue support");
  }

  commandPool = std::make_unique<core::CommandPool>(*device, ctx.commandPool);
  profiler = std::make_unique<Profiler>(*device, *commandPool, ctx.profiler);
  executor = std::make_unique<ComputeExecutor>(*device, *commandPool);

  createResources();

  graph = std::make_unique<ComputeGraph>();
  buildGraph();
  graph->compile();
  graph->create();
}

auto ComputeApplication::execute(bool capture) -> ProfileReport {
  if (capture) {
    executor->setProfiler(*profiler);
  } else {
    executor->clearProfiler();
  }

  executor->begin();
  if (capture) {
    timer->reset();
  }
  executor->beginProfileScope("compute_graph");
  graph->record();
  executor->endProfileScope();

  double recordMs = 0.0;
  if (capture) {
    timer->update();
    recordMs = timer->elapsedMilliseconds();
    timer->reset();
  }

  executor->submitAndWait();
  double submitWaitMs = 0.0;
  if (capture) {
    timer->update();
    submitWaitMs = timer->elapsedMilliseconds();
  }
  executor->end();

  if (!capture) {
    return {};
  }

  auto report = profiler->collect();
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
