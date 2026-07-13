#include "vkr/render/passes/ui.hh"
#include "vkr/logger.hh"
#include "vkr/render/renderer.hh"

namespace vkr::render {

UiPass::UiPass(Renderer &renderer, const core::Window &window,
               const core::Instance &instance, const core::Surface &surface,
               const core::Device &device, const core::CommandPool &commandPool,
               const core::Swapchain &swapchain,
               resource::ResourceManager &resourceManager, RasterPass &source,
               RenderGraph &renderGraph, util::Timer &timer,
               ui::ThemeDesc &theme)
    : renderer_(renderer), window_(window), instance_(instance),
      surface_(surface), device_(device), command_pool_(commandPool),
      swapchain_(swapchain), resource_manager_(resourceManager),
      source_(source), render_graph_(renderGraph), timer_(timer),
      theme_(theme) {}

UiPass::~UiPass() { destroy(); }

void UiPass::create() {
  destroy();

  target_ = std::make_unique<resource::SwapchainTarget>(device_, command_pool_,
                                                        swapchain_);
  target_->update(desc_.target);

  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(pipeline::RenderPassDesc::makeSwapchain(
      target_->format(), target_->depth() ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED));

  resource::FramebufferDesc framebufferDesc{.width = target_->width(),
                                            .height = target_->height(),
                                            .layers = 1,
                                            .attachments =
                                                target_->attachmentViews()};

  framebuffers_ =
      std::make_unique<resource::FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(desc_.descriptorPool);

  ui_ = std::make_unique<ui::UI>(
      window_, instance_, surface_, device_, command_pool_, resource_manager_,
      source_.target(), *render_pass_, *descriptor_pool_, render_graph_, timer_,
      theme_);
}

void UiPass::destroy() {
  ui_.reset();
  descriptor_pool_.reset();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void UiPass::update(const UiPassDesc &desc) {
  desc_ = desc;
}

void UiPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_ || !ui_) {
    VKR_RENDER_ERROR("UiPass '{}' recorded before create", name());
  }

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = renderer_.imageIndex(),
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  renderer_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  renderer_.setViewportAndScissor({target_->width(), target_->height()});
  renderer_.drawUI(*ui_);
  renderer_.endPass();
}

} // namespace vkr::render
