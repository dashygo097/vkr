#include "vkr/exec/compute/app.hh"
#include <algorithm>
#include <functional>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace vkr::exec {

namespace {

void configureComputeApp(ComputeAppDesc &ctx) {
  ctx.instance.surfaceIntegration = core::SurfaceIntegration::None;
  ctx.commandPool.queueRole = core::CommandQueueRole::Compute;
}

void logProfileReport(const ProfileReport &report) {
  if (report.empty()) {
    VKR_EXEC_INFO("profile report: no samples");
    return;
  }

  if (!report.cpuSamples.empty()) {
    VKR_EXEC_INFO("CPU profile report:");
    for (const auto &sample : report.cpuSamples) {
      VKR_EXEC_INFO("  {}: captures={}, min={:.6f} ms, mean={:.6f} ms, "
                    "median={:.6f} ms, max={:.6f} ms",
                    sample.name, sample.captureCount, sample.minMilliseconds,
                    sample.milliseconds, sample.medianMilliseconds,
                    sample.maxMilliseconds);
    }
  }

  if (!report.gpuSamples.empty()) {
    VKR_EXEC_INFO("GPU profile report:");
    for (const auto &sample : report.gpuSamples) {
      VKR_EXEC_INFO("  {}: captures={}, min={:.6f} ms, mean={:.6f} ms, "
                  "median={:.6f} ms, max={:.6f} ms",
                    sample.name, sample.captureCount, sample.minMilliseconds,
                    sample.milliseconds, sample.medianMilliseconds,
                    sample.maxMilliseconds);
    }
  }
}

auto aggregateSamples(const std::vector<ProfileReport> &reports, bool gpu)
    -> std::vector<ProfileSample> {
  std::unordered_map<std::string, std::vector<double>> timings{};
  for (const auto &report : reports) {
    const auto &samples = gpu ? report.gpuSamples : report.cpuSamples;
    for (const auto &sample : samples) {
      timings[sample.name].push_back(sample.milliseconds);
    }
  }

  std::vector<ProfileSample> aggregate{};
  aggregate.reserve(timings.size());
  for (auto &[name, values] : timings) {
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

    aggregate.push_back(ProfileSample{
        .name = name,
        .milliseconds = mean,
        .minMilliseconds = values.front(),
        .medianMilliseconds = median,
        .maxMilliseconds = values.back(),
        .captureCount = static_cast<uint32_t>(values.size()),
    });
  }

  std::sort(aggregate.begin(), aggregate.end(),
            [](const ProfileSample &lhs, const ProfileSample &rhs) {
              return lhs.name < rhs.name;
            });

  return aggregate;
}

auto aggregateProfileReports(const std::vector<ProfileReport> &reports)
    -> ProfileReport {
  ProfileReport aggregate{};

  for (const auto &report : reports) {
    aggregate.gpuTimestampsEnabled =
        aggregate.gpuTimestampsEnabled || report.gpuTimestampsEnabled;
  }

  aggregate.gpuSamples = aggregateSamples(reports, true);
  aggregate.cpuSamples = aggregateSamples(reports, false);

  return aggregate;
}

auto timeMilliseconds(util::Timer &timer, const std::function<void()> &fn)
    -> double {
  timer.reset();
  fn();
  timer.update();
  return timer.elapsedMilliseconds();
}

} // namespace

void ComputeApplication::run() {
  initCompute();
  execute();
}

void ComputeApplication::initCompute() {
  Logger::init();
  configure();
  configureComputeApp(ctx);

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
    const double recordMs = timeMilliseconds(*timer, [&]() {
      executor->beginProfileScope("compute_graph");
      graph->record();
      executor->endProfileScope();
    });

    const double submitWaitMs = timeMilliseconds(*timer, [&]() {
      executor->submitAndWait();
    });
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

  profileReport = aggregateProfileReports(captures);
  if (ctx.profiler.logReport) {
    logProfileReport(profileReport);
  }

  device->waitIdle();
  afterExecute();
}

} // namespace vkr::exec
