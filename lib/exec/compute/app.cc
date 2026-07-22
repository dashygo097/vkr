#include "vkr/exec/compute/app.hh"
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace vkr::exec {

void ComputeApplication::run() {
  initCompute();
  execute();
}

void ComputeApplication::initCompute() {
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
  executor->setProfiler(profiler.get());

  createResources();

  graph = std::make_unique<ComputeGraph>();
  buildGraph();
  graph->compile();
  graph->create();
}

void ComputeApplication::execute() {
  for (uint32_t i = 0; i < ctx.profiler.warmupFrames; ++i) {
    executor->setProfiler(nullptr);
    executor->begin();
    graph->record();
    executor->submitAndWait();
    executor->end();
  }

  std::vector<ProfileReport> captures{};
  captures.reserve(ctx.profiler.captureFrames);

  executor->setProfiler(profiler.get());
  for (uint32_t i = 0; i < ctx.profiler.captureFrames; ++i) {
    ProfileReport capture{};

    executor->begin();
    timer->reset();
    executor->beginProfileScope("compute_graph");
    graph->record();
    executor->endProfileScope();
    timer->update();
    const double recordMs = timer->elapsedMilliseconds();

    timer->reset();
    executor->submitAndWait();
    timer->update();
    const double submitWaitMs = timer->elapsedMilliseconds();
    executor->end();

    capture = profiler ? profiler->collect() : ProfileReport{};
    capture.cpuSamples.push_back(ProfileSample{
        .name = "compute_graph.record",
        .milliseconds = recordMs,
        .minMilliseconds = recordMs,
        .medianMilliseconds = recordMs,
        .maxMilliseconds = recordMs,
        .captureCount = 1,
    });
    capture.cpuSamples.push_back(ProfileSample{
        .name = "compute_graph.submit_wait",
        .milliseconds = submitWaitMs,
        .minMilliseconds = submitWaitMs,
        .medianMilliseconds = submitWaitMs,
        .maxMilliseconds = submitWaitMs,
        .captureCount = 1,
    });

    captures.push_back(std::move(capture));
  }

  profileReport = {};
  std::unordered_map<std::string, std::vector<double>> cpuTimings{};
  std::unordered_map<std::string, std::vector<double>> gpuTimings{};
  for (const auto &capture : captures) {
    profileReport.gpuTimestampsEnabled =
        profileReport.gpuTimestampsEnabled || capture.gpuTimestampsEnabled;

    for (const auto &sample : capture.cpuSamples) {
      cpuTimings[sample.name].push_back(sample.milliseconds);
    }
    for (const auto &sample : capture.gpuSamples) {
      gpuTimings[sample.name].push_back(sample.milliseconds);
    }
  }

  profileReport.cpuSamples.reserve(cpuTimings.size());
  for (auto &[name, values] : cpuTimings) {
    if (values.empty()) {
      continue;
    }

    std::sort(values.begin(), values.end());
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    const double mean = sum / static_cast<double>(values.size());
    double median = values[values.size() / 2];
    if (values.size() % 2 == 0) {
      median = (values[(values.size() / 2) - 1] + median) * 0.5;
    }

    profileReport.cpuSamples.push_back(ProfileSample{
        .name = name,
        .milliseconds = mean,
        .minMilliseconds = values.front(),
        .medianMilliseconds = median,
        .maxMilliseconds = values.back(),
        .captureCount = static_cast<uint32_t>(values.size()),
    });
  }

  profileReport.gpuSamples.reserve(gpuTimings.size());
  for (auto &[name, values] : gpuTimings) {
    if (values.empty()) {
      continue;
    }

    std::sort(values.begin(), values.end());
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    const double mean = sum / static_cast<double>(values.size());
    double median = values[values.size() / 2];
    if (values.size() % 2 == 0) {
      median = (values[(values.size() / 2) - 1] + median) * 0.5;
    }

    profileReport.gpuSamples.push_back(ProfileSample{
        .name = name,
        .milliseconds = mean,
        .minMilliseconds = values.front(),
        .medianMilliseconds = median,
        .maxMilliseconds = values.back(),
        .captureCount = static_cast<uint32_t>(values.size()),
    });
  }

  std::sort(profileReport.cpuSamples.begin(), profileReport.cpuSamples.end(),
            [](const ProfileSample &lhs, const ProfileSample &rhs) -> bool {
              return lhs.name < rhs.name;
            });
  std::sort(profileReport.gpuSamples.begin(), profileReport.gpuSamples.end(),
            [](const ProfileSample &lhs, const ProfileSample &rhs) -> bool {
              return lhs.name < rhs.name;
            });

  if (ctx.profiler.logReport) {
    if (profileReport.empty()) {
      VKR_EXEC_INFO("profile report: no samples");
    }

    if (!profileReport.cpuSamples.empty()) {
      VKR_EXEC_INFO("CPU profile report:");
      for (const auto &sample : profileReport.cpuSamples) {
        VKR_EXEC_INFO("  {}: captures={}, min={:.6f} ms, mean={:.6f} ms, "
                      "median={:.6f} ms, max={:.6f} ms",
                      sample.name, sample.captureCount, sample.minMilliseconds,
                      sample.milliseconds, sample.medianMilliseconds,
                      sample.maxMilliseconds);
      }
    }

    if (!profileReport.gpuSamples.empty()) {
      VKR_EXEC_INFO("GPU profile report:");
      for (const auto &sample : profileReport.gpuSamples) {
        VKR_EXEC_INFO("  {}: captures={}, min={:.6f} ms, mean={:.6f} ms, "
                      "median={:.6f} ms, max={:.6f} ms",
                      sample.name, sample.captureCount, sample.minMilliseconds,
                      sample.milliseconds, sample.medianMilliseconds,
                      sample.maxMilliseconds);
      }
    }
  }

  device->waitIdle();
  afterExecute();
}

} // namespace vkr::exec
