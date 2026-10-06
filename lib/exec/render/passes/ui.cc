#include "vkr/exec/render/passes/ui.hh"
#include "vkr/exec/capability.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/logger.hh"
#include <imgui_impl_vulkan.h>

namespace vkr::exec {

UiPass::UiPass(RenderExecutor &executor, const core::Window &window,
               const core::Instance &instance, const core::Surface &surface,
               const core::Device &device, const core::CommandPool &commandPool,
               const core::CommandBuffers &commandBuffers,
               const core::Swapchain &swapchain, scene::Scene &scene,
               const util::AssetSystem &assetSystem, scene::CameraDesc &camera,
               Pass &source, Graph &graph, util::Timer &timer,
               ui::UiDesc &uiDesc)
    : executor_(executor), window_(window), instance_(instance),
      surface_(surface), device_(device), command_pool_(commandPool),
      command_buffers_(commandBuffers), swapchain_(swapchain), scene_(scene),
      asset_system_(assetSystem), camera_(camera), source_(source),
      graph_(graph), timer_(timer), ui_desc_(uiDesc) {}

UiPass::~UiPass() { destroy(); }

void UiPass::create() {
  destroy();

  const auto source = source_.capability<RenderTargetCapability>();
  if (!source) {
    VKR_EXEC_ERROR("UiPass '{}' source '{}' has no render target", name(),
                   source_.name());
  }
  const auto &sourceTarget = source->get().target(0);
  if (!sourceTarget.hasColor() || !sourceTarget.color().hasSampler()) {
    VKR_EXEC_ERROR("UiPass '{}' source '{}' needs a sampled color target",
                   name(), source_.name());
  }

  target_ =
      std::make_unique<SwapchainTarget>(device_, command_pool_, swapchain_);
  target_->update(SwapchainTargetDesc{});

  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(pipeline::RenderPassDesc::makeSwapchain(
      target_->format(), target_->depth() ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED));

  FramebufferDesc framebufferDesc{.width = target_->width(),
                                  .height = target_->height(),
                                  .layers = 1,
                                  .attachments = target_->attachmentViews()};

  framebuffers_ = std::make_unique<FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  // Viewport and selected-texture preview each use one set per frame slot.
  // Leave the backend's documented reservation for its dynamic font atlas.
  const uint32_t descriptorCount =
      command_buffers_.size() * 2U +
      IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
  descriptor_pool_->update({
      .poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount}},
      .maxSets = descriptorCount,
  });

  ui_ = std::make_unique<ui::UI>(window_, instance_, surface_, device_,
                                 command_pool_, scene_, asset_system_, camera_,
                                 source_, *render_pass_, *descriptor_pool_,
                                 graph_, timer_, ui_desc_, command_buffers_);
}

void UiPass::destroy() noexcept {
  ui_.reset();
  descriptor_pool_.reset();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void UiPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_ || !ui_) {
    VKR_EXEC_ERROR("UiPass '{}' recorded before create", name());
  }

  executor_.beginProfileScope(name());
  executor_.beginPass(*framebuffers_,
                      {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}},
                      executor_.imageIndex());
  executor_.setViewportAndScissor({target_->width(), target_->height()});
  ui_->render(executor_.commandBuffer(), executor_.frameIndex());
  executor_.endPass();
  executor_.endProfileScope();
}

} // namespace vkr::exec
