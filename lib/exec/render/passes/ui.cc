#include "vkr/exec/render/passes/ui.hh"
#include "vkr/logger.hh"
#include "vkr/exec/render/executor.hh"

namespace vkr::exec {

UiPass::UiPass(Executor &executor, const core::Window &window,
               const core::Instance &instance, const core::Surface &surface,
               const core::Device &device, const core::CommandPool &commandPool,
               const core::Swapchain &swapchain,
               scene::Scene &scene,
               const util::AssetSystem &assetSystem, scene::CameraDesc &camera,
               FullscreenPassSource source, RenderGraph &graph,
               util::Timer &timer,
               ui::UiDesc &uiDesc)
    : executor_(executor), window_(window), instance_(instance),
      surface_(surface), device_(device), command_pool_(commandPool),
      swapchain_(swapchain), scene_(scene),
      asset_system_(assetSystem), camera_(camera), source_(source),
      graph_(graph), timer_(timer), ui_desc_(uiDesc) {}

UiPass::~UiPass() { destroy(); }

void UiPass::create() {
  destroy();

  target_ = std::make_unique<SwapchainTarget>(device_, command_pool_,
                                                        swapchain_);
  target_->update(desc_.target);

  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(pipeline::RenderPassDesc::makeSwapchain(
      target_->format(), target_->depth() ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED));

  FramebufferDesc framebufferDesc{.width = target_->width(),
                                            .height = target_->height(),
                                            .layers = 1,
                                            .attachments =
                                                target_->attachmentViews()};

  framebuffers_ =
      std::make_unique<FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(desc_.descriptorPool);

  ui_ = std::make_unique<ui::UI>(
      window_, instance_, surface_, device_, command_pool_, scene_,
      asset_system_, camera_, source_.target(), *render_pass_,
      *descriptor_pool_, graph_, timer_, ui_desc_, executor_.framesInFlight());
  ui_->layoutMode(desc_.layoutMode);
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
    VKR_EXEC_ERROR("UiPass '{}' recorded before create", name());
  }

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = executor_.imageIndex(),
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  executor_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  executor_.setViewportAndScissor({target_->width(), target_->height()});
  executor_.drawUI(*ui_);
  executor_.endPass();
}

} // namespace vkr::exec
