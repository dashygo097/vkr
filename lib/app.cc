#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <functional>
#include <optional>
#include <typeinfo>
#include <vulkan/vulkan_core.h>

namespace vkr {

namespace {

auto findUiPass(render::RenderGraph &graph)
    -> std::optional<std::reference_wrapper<render::UiPass>> {
  for (auto pass : graph.passes()) {
    auto &graphPass = pass.get();

    if (typeid(graphPass) == typeid(render::UiPass)) {
      return static_cast<render::UiPass &>(graphPass);
    }
  }

  return std::nullopt;
}

auto findUiPass(const render::RenderGraph &graph)
    -> std::optional<std::reference_wrapper<const render::UiPass>> {
  for (auto pass : graph.passes()) {
    const auto &graphPass = pass.get();

    if (typeid(graphPass) == typeid(render::UiPass)) {
      return static_cast<const render::UiPass &>(graphPass);
    }
  }

  return std::nullopt;
}

} // namespace

void VulkanApplication::initVulkan() {
  Logger::init();
  configure();
  loadSnapshot();

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

  // swapchain
  swapchain = std::make_unique<core::Swapchain>(*window, *surface, *device,
                                                ctx.swapchain);

  // command pool
  commandPool = std::make_unique<core::CommandPool>(*device);

  // sync objects
  syncObjects = std::make_unique<core::SyncObjects>(*device, *swapchain);

  // resource manager
  resourceManager =
      std::make_unique<resource::ResourceManager>(*device, *commandPool);

  // user resources
  createResources();

  // timer
  timer = std::make_unique<util::Timer>();

  // camera
  camera = std::make_unique<scene::Camera>(*window, *timer, *inputTracer,
                                           ctx.camera);

  // renderer
  renderer = std::make_unique<render::Renderer>(
      *device, *swapchain, *commandPool, *syncObjects, *resourceManager);

  // render graph
  renderGraph = std::make_unique<render::RenderGraph>();
  buildRenderGraph();
  renderGraph->compile();
  renderGraph->create();
}

void VulkanApplication::mainLoop() {
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

void VulkanApplication::drawFrame() {
  if (!renderer->beginFrame()) {
    if (renderer->consumeSwapchainOutOfDate()) {
      recreateSwapchain();
    }
    return;
  }

  onDraw();
  renderGraph->record();

  renderer->submitFrame();
  renderGraph->present();
  renderGraph->afterFrame();
  renderer->endFrame();

  if (renderer->consumeSwapchainOutOfDate()) {
    recreateSwapchain();
  }
}

auto VulkanApplication::shouldClose() const -> bool {
  if (!renderGraph) {
    return false;
  }

  const auto uiPass = findUiPass(*renderGraph);
  return uiPass && uiPass->get().shouldClose();
}

void VulkanApplication::updateUiState() {
  const auto uiPass = findUiPass(*renderGraph);
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

  const bool lockCamera = ctx.ui.layoutMode == ui::LayoutMode::Standard &&
                          !ctx.ui.viewportFocused;
  camera->lock(lockCamera);
}

void VulkanApplication::recreateSwapchain() {
  if (renderGraph) {
    const auto uiPass = findUiPass(*renderGraph);
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

  renderGraph->destroy();

  swapchain->recreate();
  syncObjects->recreate();

  renderGraph = std::make_unique<render::RenderGraph>();
  buildRenderGraph();
  renderGraph->compile();
  renderGraph->create();
}

} // namespace vkr
