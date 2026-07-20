#include "vkr/exec/compute/app.hh"

namespace vkr::exec {

namespace {

void configureComputeApp(ComputeAppDesc &ctx) {
  ctx.instance.surfaceIntegration = core::SurfaceIntegration::None;
  ctx.commandPool.queueRole = core::CommandQueueRole::Compute;
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
  executor = std::make_unique<ComputeExecutor>(*device, *commandPool);

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

  device->waitIdle();
  afterExecute();
}

} // namespace vkr::exec
