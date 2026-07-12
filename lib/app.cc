#include "vkr/app.hh"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace vkr {

void VulkanApplication::initVulkan() {
  Logger::init();
  onConfigure();
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

  // render graph
  renderGraph = std::make_unique<render::RenderGraph>();
  rasterPass = &renderGraph->addPass<render::RasterPass>(
      *device, *commandPool, *resourceManager, createRasterPassDesc());
  uiPass = &renderGraph->addPass<render::UiPass>(
      *window, *instance, *surface, *device, *commandPool, *swapchain,
      *resourceManager, *rasterPass, *timer, ctx.theme);
  renderGraph->addPass<render::PresentPass>();
  renderGraph->compile();
  renderGraph->create();

  // renderer
  renderer = std::make_unique<render::Renderer>(
      *device, *swapchain, *commandPool, *syncObjects, *resourceManager);
}

void VulkanApplication::mainLoop() {
  timer->start();
  bool isLastTabKeyPressed = false;

  while (!window->shouldClose() && !uiPass->shouldClose()) {
    timer->beginFrame();

    window->pollEvents();

    bool isNowTabKeyPressed =
        glfwGetKey(window->glfwWindow(), GLFW_KEY_TAB) == GLFW_PRESS;

    if (!camera->isLocked()) {
      camera->track();
    }

    if (isNowTabKeyPressed && !isLastTabKeyPressed) {
      uiPass->switchLayoutMode();
    }

    bool shouldLockCamera = false;
    if (uiPass->layoutMode() == ui::LayoutMode::Standard) {
      shouldLockCamera = !uiPass->viewportInfo().isHovered;
    }

    camera->lock(shouldLockCamera);

    timer->update();
    drawFrame();

    isLastTabKeyPressed = isNowTabKeyPressed;
    timer->endFrame();
  }

  device->waitIdle();
}

void VulkanApplication::drawFrame() {
  if (!renderer->beginFrame()) {
    return;
  }

  onDrawFrame(renderer->frameIndex());
  renderGraph->record(*renderer);

  renderer->endFrame();
}

auto VulkanApplication::createRasterPassDesc() -> render::RasterPassDesc {
  render::RasterPassDesc desc{};
  desc.target = resource::OffscreenTargetDesc{
      .color = {.width = swapchain->width(),
                .height = swapchain->height(),
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                         VK_IMAGE_USAGE_SAMPLED_BIT,
                .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .createSampler = true},
      .depth = resource::DepthAttachmentDesc{.width = swapchain->width(),
                                             .height = swapchain->height(),
                                             .format = VK_FORMAT_D32_SFLOAT}};
  return desc;
}

} // namespace vkr
