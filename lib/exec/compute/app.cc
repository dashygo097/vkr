#include "vkr/exec/compute/app.hh"

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
    VKR_EXEC_INFO("  {}: {:.6f} ms", sample.name, sample.gpuMilliseconds);
  }
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
  executor->begin();
  graph->record();
  executor->submitAndWait();
  executor->end();
  profileReport = profiler ? profiler->collect() : ProfileReport{};
  logProfileReport(profileReport);

  device->waitIdle();
  afterExecute();
}

} // namespace vkr::exec
