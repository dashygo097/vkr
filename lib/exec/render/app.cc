#include "vkr/exec/render/app.hh"
#include <GLFW/glfw3.h>
#include <functional>
#include <optional>
#include <typeinfo>

namespace vkr::exec {

namespace {

auto findUiPass(RenderGraph &graph)
    -> std::optional<std::reference_wrapper<UiPass>> {
  for (auto pass : graph.passes()) {
    auto &graphPass = pass.get();

    if (typeid(graphPass) == typeid(UiPass)) {
      return static_cast<UiPass &>(graphPass);
    }
  }

  return std::nullopt;
}

auto findUiPass(const RenderGraph &graph)
    -> std::optional<std::reference_wrapper<const UiPass>> {
  for (auto pass : graph.passes()) {
    const auto &graphPass = pass.get();

    if (typeid(graphPass) == typeid(UiPass)) {
      return static_cast<const UiPass &>(graphPass);
    }
  }

  return std::nullopt;
}

void configureCommandPoolRoles(RenderAppDesc &ctx) {
  ctx.graphicsCommandPool.queueRole = core::CommandQueueRole::Graphics;
  ctx.computeCommandPool.queueRole = core::CommandQueueRole::Compute;
  ctx.transferCommandPool.queueRole = core::CommandQueueRole::Transfer;
}

} // namespace

void RenderApplication::initVulkan() {
  Logger::init();
  configure();
  loadSnapshot();
  configureCommandPoolRoles(ctx);

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
  graphicsCommandPool =
      std::make_unique<core::CommandPool>(*device, ctx.graphicsCommandPool);
  if (device->supportsCompute()) {
    computeCommandPool =
        std::make_unique<core::CommandPool>(*device, ctx.computeCommandPool);
  } else {
    VKR_CORE_WARN("compute queue is not supported; compute command pool "
                  "will not be initialized");
  }
  if (device->supportsTransfer()) {
    transferCommandPool =
        std::make_unique<core::CommandPool>(*device, ctx.transferCommandPool);
  } else {
    VKR_CORE_WARN("transfer queue is not supported; transfer command pool "
                  "will not be initialized");
  }

  // sync objects
  frameSync = std::make_unique<FrameSync>(*device, *swapchain);

  // scene
  scene = std::make_unique<vkr::scene::Scene>(*device, *graphicsCommandPool);

  // user resources
  createResources();

  // timer
  timer = std::make_unique<util::Timer>();

  // camera
  camera =
      std::make_unique<vkr::scene::Camera>(*timer, *inputTracer, ctx.camera);

  // executor
  executor = std::make_unique<Executor>(*device, *swapchain,
                                        *graphicsCommandPool, *frameSync,
                                        *scene);

  // render graph
  graph = std::make_unique<RenderGraph>();
  buildGraph();
  graph->compile();
  graph->create();
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
  graph->record();

  executor->submitFrame();
  graph->present();
  graph->afterFrame();
  executor->endFrame();

  if (executor->consumeSwapchainOutOfDate()) {
    recreateSwapchain();
  }
}

auto RenderApplication::shouldClose() const -> bool {
  if (!graph) {
    return false;
  }

  const auto uiPass = findUiPass(*graph);
  return uiPass && uiPass->get().shouldClose();
}

void RenderApplication::updateUiState() {
  const auto uiPass = findUiPass(*graph);
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
    const auto uiPass = findUiPass(*graph);
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

  ctx.camera.aspectRatio = ctx.window.ratio();

  graph->destroy();

  swapchain->recreate();
  frameSync->recreate();

  graph = std::make_unique<RenderGraph>();
  buildGraph();
  graph->compile();
  graph->create();
}

} // namespace vkr::exec
