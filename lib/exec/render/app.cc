#include "vkr/exec/render/app.hh"
#include "vkr/exec/render/passes/present.hh"
#include "vkr/exec/render/passes/ui.hh"
#include "vkr/logger.hh"
#include "vkr/util/toml.hh"
#include <GLFW/glfw3.h>
#include <filesystem>

namespace vkr::exec {

auto RenderAppDesc::windowed(std::string appName, std::string windowTitle,
                             uint32_t width, uint32_t height,
                             uint32_t framesInFlight) -> RenderAppDesc {
  RenderAppDesc desc{};
  desc.window = {
      .title = std::move(windowTitle),
      .width = width,
      .height = height,
  };
  desc.instance = {
      .name = std::move(appName),
      .version = VK_MAKE_VERSION(1, 0, 0),
      .surfaceIntegration = core::SurfaceIntegration::GLFW,
  };
  desc.commandBuffers.size = framesInFlight;
  return desc;
}

void RenderApplication::run() {
  initVulkan();

  try {
    mainLoop();
    saveSnapshot();
  } catch (...) {
    saveSnapshot();
    throw;
  }
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
  const auto path = snapshotPath();

  if (!vkr::util::saveTomlFile(path, ctx)) {
    VKR_UTIL_WARN("failed to save snapshot: {}", path.string());
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
  camera =
      std::make_unique<vkr::scene::Camera>(*timer, *inputTracer, ctx.camera);

  // executor
  executor = std::make_unique<RenderExecutor>(*device, *swapchain, *commandPool,
                                              *scene, *commandBuffers);
  executor->setProfiler(*profiler);

  // render graph
  graph =
      std::make_unique<RenderGraph>(*executor, *device, *commandPool, *scene);
  buildGraph();
  buildPresentation();
  graph->compile();
  graph->create();
}

void RenderApplication::buildPresentation() {
  const auto &source = graph->presentationSource();

  auto &uiPass = graph->addPass<UiPass>(
      *executor, *window, *instance, *surface, *device, *commandPool,
      *commandBuffers, *swapchain, *scene, *assetSystem, ctx.camera, source,
      *graph, *timer, ctx.ui);
  uiPass.setName("ui");
  graph->addDependency(source.name(), uiPass.name());

  auto &presentPass = graph->addPass<PresentPass>(*executor);
  presentPass.setName("present");
  graph->addDependency(uiPass.name(), presentPass.name());
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

    updateUiState();

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

auto RenderApplication::shouldClose() const -> bool {
  if (!graph) {
    return false;
  }

  const auto uiPass = graph->uiPass();
  return uiPass && uiPass->get().shouldClose();
}

void RenderApplication::updateUiState() {
  const auto uiPass = graph->uiPass();
  if (!uiPass) {
    return;
  }

  if (inputTracer->wasKeyPressed(GLFW_KEY_TAB)) {
    uiPass->get().switchLayoutMode();
  }

  ctx.ui.layoutMode = uiPass->get().layoutMode();
  ctx.ui.viewport = uiPass->get().viewport();
  ctx.ui.viewportFocused = uiPass->get().viewportFocused();
  ctx.ui.viewportHovered = uiPass->get().viewportHovered();

  const bool lockCamera =
      ctx.ui.layoutMode == ui::LayoutMode::Standard && !ctx.ui.viewportFocused;
  camera->lock(lockCamera);
}

void RenderApplication::recreateSwapchain() {
  if (graph) {
    const auto uiPass = graph->uiPass();
    if (uiPass) {
      ctx.ui.layoutMode = uiPass->get().layoutMode();
    }
  }

  device->waitIdle();
  window->waitForFramebufferSize();
  const bool ignoredResizeFlag = window->consumeFramebufferResized();
  (void)ignoredResizeFlag;

  if (window->shouldClose()) {
    return;
  }

  graph->destroy();
  graph.reset();

  swapchain->recreate();

  executor = std::make_unique<RenderExecutor>(*device, *swapchain, *commandPool,
                                              *scene, *commandBuffers);
  executor->setProfiler(*profiler);

  graph =
      std::make_unique<RenderGraph>(*executor, *device, *commandPool, *scene);
  buildGraph();
  buildPresentation();
  graph->compile();
  graph->create();
}

} // namespace vkr::exec
