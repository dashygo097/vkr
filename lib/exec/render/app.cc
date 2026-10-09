#include "vkr/exec/render/app.hh"
#include "vkr/exec/render/passes/present.hh"
#include "vkr/exec/render/passes/ui.hh"
#include "vkr/logger.hh"
#include "vkr/util/toml.hh"
#include <GLFW/glfw3.h>
#include <filesystem>

namespace vkr::exec {

void RenderApplication::run() {
  initVulkan();

  try {
    mainLoop();
    saveSnapshot();
  } catch (...) {
    device_->waitIdle();
    saveSnapshot();
    throw;
  }
}

void RenderApplication::initVulkan() {
  Logger::init();
  configure();
  loadSnapshot();

  ctx.commandPool.queueRole = core::CommandQueueRole::Graphics;

  if (!ctx.isValid()) {
    VKR_CORE_ERROR("invalid app config");
  }

  // asset
  asset_system_ = std::make_unique<util::AssetSystem>(ctx.asset);

  // window
  window_ = std::make_unique<core::Window>(ctx.window);

  // input
  input_tracer_ = std::make_unique<util::InputTracer>(window_->glfwWindow());
  input_tracer_->installCallbacks();

  // instance
  instance_ = std::make_unique<core::Instance>(ctx.instance);

  // surface
  surface_ = std::make_unique<core::Surface>(*instance_, *window_);

  // device
  device_ = std::make_unique<core::Device>(*instance_, *surface_, ctx.device);
  if (!device_->supportsGraphics() || !device_->supportsPresent()) {
    VKR_CORE_ERROR("rendering requires graphics and present queue support");
  }

  // swapchain
  swapchain_ = std::make_unique<core::Swapchain>(
      *window_, *surface_, *device_, ctx.swapchain);

  // command pool
  command_pool_ = std::make_unique<core::CommandPool>(*device_, ctx.commandPool);

  // profiler
  profiler_ = std::make_unique<Profiler>(*device_, *command_pool_, ctx.profiler);

  // command buffers
  command_buffers_ =
      std::make_unique<core::CommandBuffers>(*device_, *command_pool_);
  command_buffers_->update(ctx.commandBuffers);

  // scene
  scene_ = std::make_unique<vkr::scene::Scene>(
      *device_, *command_pool_, *command_buffers_);

  // user resources
  createResources();

  // timer
  timer_ = std::make_unique<util::Timer>();

  // camera
  camera_ = std::make_unique<vkr::scene::Camera>(*timer_, *input_tracer_);
  camera_->update(ctx.camera);

  // executor
  executor_ = std::make_unique<RenderExecutor>(
      *device_, *swapchain_, *command_pool_, *scene_, *command_buffers_);
  executor_->setProfiler(*profiler_);

  // render graph
  graph_ =
      std::make_unique<RenderGraph>(*executor_, *device_, *command_pool_, *scene_);
  buildGraph();
  auto &uiPass = buildPresentation();
  graph_->compile();
  graph_->create();
  ui_ = uiPass.ui();
}

auto RenderApplication::buildPresentation() -> UiPass & {
  auto &source = graph_->presentationSource();

  auto &uiPass = graph_->addPass<UiPass>(
      *executor_, *window_, *instance_, *device_, *command_buffers_, *swapchain_,
      *scene_, *asset_system_, *camera_, source, *graph_, *timer_);
  uiPass.setName("ui");
  uiPass.update(ctx.ui);
  graph_->addDependency(source.name(), uiPass.name());

  auto &presentPass = graph_->addPass<PresentPass>(*executor_);
  presentPass.setName("present");
  graph_->addDependency(uiPass.name(), presentPass.name());
  return uiPass;
}

void RenderApplication::mainLoop() {
  timer_->start();
  while (!window_->shouldClose() && !shouldClose()) {
    timer_->beginFrame();
    input_tracer_->beginFrame();

    window_->pollEvents();
    input_tracer_->update();

    if (window_->consumeFramebufferResized()) {
      recreateSwapchain();
    }

    camera_->lock(ui().layoutMode() == ui::LayoutMode::Standard &&
                 !ui().viewportFocused());

    if (!camera_->isLocked()) {
      camera_->track();
    }

    timer_->update();
    drawFrame();

    timer_->endFrame();
  }

  device_->waitIdle();
}

void RenderApplication::drawFrame() {
  if (!executor_->beginFrame()) {
    if (executor_->consumeSwapchainOutOfDate()) {
      recreateSwapchain();
    }
    return;
  }

  onDraw();
  executor_->beginProfileScope("render_graph");
  graph_->record();
  executor_->endProfileScope();

  executor_->submitFrame();
  const auto report = profiler_ ? profiler_->collect() : ProfileReport{};
  if (ctx.profiler.logReport && !report.gpuSamples.empty()) {
    VKR_EXEC_INFO("GPU profile report:");
    for (const auto &sample : report.gpuSamples) {
      VKR_EXEC_INFO("  {}: {:.6f} ms", sample.name, sample.milliseconds);
    }
  }

  graph_->present();
  executor_->endFrame();

  if (executor_->consumeSwapchainOutOfDate()) {
    recreateSwapchain();
  }
}

void RenderApplication::recreateSwapchain() {
  device_->waitIdle();
  window_->waitForFramebufferSize();
  const bool ignoredResizeFlag = window_->consumeFramebufferResized();
  (void)ignoredResizeFlag;

  if (window_->shouldClose()) {
    return;
  }

  ctx.ui = ui().desc();
  ctx.ui.layoutMode = ui().layoutMode();
  ctx.ui.theme = ui().theme();
  ui_.reset();
  graph_->destroy();
  graph_.reset();

  swapchain_->recreate();

  executor_ = std::make_unique<RenderExecutor>(
      *device_, *swapchain_, *command_pool_, *scene_, *command_buffers_);
  executor_->setProfiler(*profiler_);

  graph_ =
      std::make_unique<RenderGraph>(*executor_, *device_, *command_pool_, *scene_);
  buildGraph();
  auto &uiPass = buildPresentation();
  graph_->compile();
  graph_->create();
  ui_ = uiPass.ui();
}

void RenderApplication::loadSnapshot() {
  const auto path = snapshotPath();

  if (!std::filesystem::exists(path)) {
    VKR_UTIL_INFO("snapshot not found, using default config: {}",
                  path.string());
    return;
  }

  if (!vkr::util::loadTomlFile(path, ctx)) {
    VKR_UTIL_WARN("failed to load snapshot, using default config: {}",
                  path.string());
  }
}

void RenderApplication::saveSnapshot() {
  if (camera_ && camera_->valid()) {
    ctx.camera = camera_->desc();
  }
  if (ui_) {
    ctx.ui = ui().desc();
    ctx.ui.layoutMode = ui().layoutMode();
    ctx.ui.theme = ui().theme();
  }
  const auto path = snapshotPath();

  if (!vkr::util::saveTomlFile(path, ctx)) {
    VKR_UTIL_WARN("failed to save snapshot: {}", path.string());
  }
}

} // namespace vkr::exec
