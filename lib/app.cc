#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace vkr {

void VulkanApplication::initVulkan() {
  Logger::init();
  configure();
  loadSnapshot();

  if (!ctx.isValid()) {
    VKR_CORE_ERROR("invalid Vulkan context config");
  }

  // asset
  assetSystem = std::make_unique<util::AssetSystem>(ctx.asset);

  // window
  window = std::make_unique<core::Window>(ctx.window);
  glfwSetWindowUserPointer(window->glfwWindow(), this);

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
  camera = std::make_unique<scene::Camera>(*window, *timer, ctx.camera);

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

    window->pollEvents();

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
    return;
  }

  onDraw();
  renderGraph->record();

  renderer->endFrame();
}

void VulkanApplication::updateUiState() {
  if (!uiPass_) {
    return;
  }

  static bool wasTabPressed = false;
  const bool isTabPressed =
      glfwGetKey(window->glfwWindow(), GLFW_KEY_TAB) == GLFW_PRESS;

  if (isTabPressed && !wasTabPressed) {
    uiPass_->switchLayoutMode();
  }

  wasTabPressed = isTabPressed;

  const bool lockCamera = uiPass_->layoutMode() == ui::LayoutMode::Standard &&
                          !uiPass_->viewportInfo().isHovered;
  camera->lock(lockCamera);
}

auto VulkanApplication::addUiPass(render::RasterPass &source)
    -> render::UiPass & {
  render::UiPassDesc desc{};
  desc.graph = {
      .name = "ui", .reads = {"scene.color"}, .writes = {"swapchain"}};
  desc.target = {};
  desc.descriptorPool = {
      .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
      .maxSets = core::MAX_FRAMES_IN_FLIGHT + 2};
  desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

  uiPass_ = &renderGraph->addPass<render::UiPass>(
      *renderer, *window, *instance, *surface, *device, *commandPool,
      *swapchain, *resourceManager, source, *timer, ctx.theme, std::move(desc));
  return *uiPass_;
}

} // namespace vkr
