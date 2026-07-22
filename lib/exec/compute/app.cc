#include "vkr/exec/compute/app.hh"
#include <algorithm>
#include <numeric>
#include <unordered_map>

namespace vkr::exec {

namespace {

void configureComputeApp(ComputeAppDesc &ctx) {
  ctx.instance.surfaceIntegration = core::SurfaceIntegration::None;
  ctx.commandPool.queueRole = core::CommandQueueRole::Compute;
}

void logProfileReport(const ProfileReport &report) {
  if (!report.gpuTimestampsEnabled) {
    return;
  }

  if (report.empty()) {
    VKR_EXEC_INFO("GPU profile report: no samples");
    return;
  }

  VKR_EXEC_INFO("GPU profile report:");
  for (const auto &sample : report.samples) {
    VKR_EXEC_INFO("  {}: captures={}, min={:.6f} ms, mean={:.6f} ms, "
                  "median={:.6f} ms, max={:.6f} ms",
                  sample.name, sample.captureCount, sample.minGpuMilliseconds,
                  sample.gpuMilliseconds, sample.medianGpuMilliseconds,
                  sample.maxGpuMilliseconds);
  }
}

auto aggregateProfileReports(const std::vector<ProfileReport> &reports)
    -> ProfileReport {
  ProfileReport aggregate{};

  for (const auto &report : reports) {
    aggregate.gpuTimestampsEnabled =
        aggregate.gpuTimestampsEnabled || report.gpuTimestampsEnabled;
  }

  if (!aggregate.gpuTimestampsEnabled) {
    return aggregate;
  }

  std::unordered_map<std::string, std::vector<double>> timings{};
  for (const auto &report : reports) {
    for (const auto &sample : report.samples) {
      timings[sample.name].push_back(sample.gpuMilliseconds);
    }
  }

  aggregate.samples.reserve(timings.size());
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

    aggregate.samples.push_back(ProfileSample{
        .name = name,
        .gpuMilliseconds = mean,
        .minGpuMilliseconds = values.front(),
        .medianGpuMilliseconds = median,
        .maxGpuMilliseconds = values.back(),
        .captureCount = static_cast<uint32_t>(values.size()),
    });
  }

  std::sort(aggregate.samples.begin(), aggregate.samples.end(),
            [](const ProfileSample &lhs, const ProfileSample &rhs) {
              return lhs.name < rhs.name;
            });

  return aggregate;
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
    executor->begin();
    graph->record();
    executor->submitAndWait();
    executor->end();
    captures.push_back(profiler ? profiler->collect() : ProfileReport{});
  }

  profileReport = aggregateProfileReports(captures);
  if (ctx.profiler.logReport) {
    logProfileReport(profileReport);
  }

  device->waitIdle();
  afterExecute();
}

} // namespace vkr::exec
