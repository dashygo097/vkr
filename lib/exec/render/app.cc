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
    device->waitIdle();
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
  assetSystem = std::make_unique<util::AssetSystem>(ctx.asset);

  // window
  window = std::make_unique<core::Window>(ctx.window);

  // input
  inputTracer = std::make_unique<util::InputTracer>(window->glfwWindow());
  inputTracer->installCallbacks();

  // instance
  instance = std::make_unique<core::Instance>(ctx.instance);

  // surface
  surface = std::make_unique<core::Surface>(*instance, *window);

  // device
  device = std::make_unique<core::Device>(*instance, *surface, ctx.device);
  if (!device->supportsGraphics() || !device->supportsPresent()) {
    VKR_CORE_ERROR("rendering requires graphics and present queue support");
  }

  // swapchain
  swapchain = std::make_unique<core::Swapchain>(*window, *surface, *device,
                                                ctx.swapchain);

  // command pool
  commandPool = std::make_unique<core::CommandPool>(*device, ctx.commandPool);

  // profiler
  profiler = std::make_unique<Profiler>(*device, *commandPool, ctx.profiler);

  // command buffers
  commandBuffers =
      std::make_unique<core::CommandBuffers>(*device, *commandPool);
  commandBuffers->update(ctx.commandBuffers);

  // scene
  scene = std::make_unique<vkr::scene::Scene>(*device, *commandPool,
                                              *commandBuffers);

  // user resources
  createResources();

  // timer
  timer = std::make_unique<util::Timer>();

  // camera
  camera = std::make_unique<vkr::scene::Camera>(*timer, *inputTracer);
  camera->update(ctx.camera);

  // executor
  executor = std::make_unique<RenderExecutor>(*device, *swapchain, *commandPool,
                                              *scene, *commandBuffers);
  executor->setProfiler(*profiler);

  // render graph
  graph =
      std::make_unique<RenderGraph>(*executor, *device, *commandPool, *scene);
  buildGraph();
  auto &uiPass = buildPresentation();
  graph->compile();
  graph->create();
  ui_ = uiPass.ui();
}

auto RenderApplication::buildPresentation() -> UiPass & {
  auto &source = graph->presentationSource();

  auto &uiPass = graph->addPass<UiPass>(
      *executor, *window, *instance, *device, *commandBuffers, *swapchain,
      *scene, *assetSystem, *camera, source, *graph, *timer);
  uiPass.setName("ui");
  uiPass.update(ctx.ui);
  graph->addDependency(source.name(), uiPass.name());

  auto &presentPass = graph->addPass<PresentPass>(*executor);
  presentPass.setName("present");
  graph->addDependency(uiPass.name(), presentPass.name());
  return uiPass;
}

void RenderApplication::mainLoop() {
  timer->start();
  while (!window->shouldClose() && !shouldClose()) {
    timer->beginFrame();
    inputTracer->beginFrame();

    window->pollEvents();
    inputTracer->update();

    if (window->consumeFramebufferResized()) {
      recreateSwapchain();
    }

    camera->lock(ui().layoutMode() == ui::LayoutMode::Standard &&
                 !ui().viewportFocused());

    if (!camera->isLocked()) {
      camera->track();
    }

    timer->update();
    drawFrame();

    timer->endFrame();
  }

  device->waitIdle();
}

void RenderApplication::drawFrame() {
  if (!executor->beginFrame()) {
    if (executor->consumeSwapchainOutOfDate()) {
      recreateSwapchain();
    }
    return;
  }

  onDraw();
  executor->beginProfileScope("render_graph");
  graph->record();
  executor->endProfileScope();

  executor->submitFrame();
  profileReport = profiler ? profiler->collect() : ProfileReport{};
  if (ctx.profiler.logReport && !profileReport.gpuSamples.empty()) {
    VKR_EXEC_INFO("GPU profile report:");
    for (const auto &sample : profileReport.gpuSamples) {
      VKR_EXEC_INFO("  {}: {:.6f} ms", sample.name, sample.milliseconds);
    }
  }

  graph->present();
  executor->endFrame();

  if (executor->consumeSwapchainOutOfDate()) {
    recreateSwapchain();
  }
}

void RenderApplication::recreateSwapchain() {
  device->waitIdle();
  window->waitForFramebufferSize();
  const bool ignoredResizeFlag = window->consumeFramebufferResized();
  (void)ignoredResizeFlag;

  if (window->shouldClose()) {
    return;
  }

  ctx.ui = ui().desc();
  ctx.ui.layoutMode = ui().layoutMode();
  ctx.ui.theme = ui().theme();
  ui_.reset();
  graph->destroy();
  graph.reset();

  swapchain->recreate();

  executor = std::make_unique<RenderExecutor>(*device, *swapchain, *commandPool,
                                              *scene, *commandBuffers);
  executor->setProfiler(*profiler);

  graph =
      std::make_unique<RenderGraph>(*executor, *device, *commandPool, *scene);
  buildGraph();
  auto &uiPass = buildPresentation();
  graph->compile();
  graph->create();
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

auto RenderApplication::ui() -> ui::UI & {
  if (!ui_) {
    VKR_EXEC_ERROR("UI requested before initialization");
  }
  return ui_->get();
}

auto RenderApplication::ui() const -> const ui::UI & {
  if (!ui_) {
    VKR_EXEC_ERROR("UI requested before initialization");
  }
  return ui_->get();
}

void RenderApplication::saveSnapshot() {
  if (camera && camera->valid()) {
    ctx.camera = camera->desc();
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
