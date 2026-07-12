#pragma once

#include "vkr/core/command/command_buffer.hh"
#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/window.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/render/pass.hh"
#include "vkr/render/passes/raster.hh"
#include "vkr/resource/attachments/frame_buffer.hh"
#include "vkr/resource/manager.hh"
#include "vkr/resource/targets/swapchain.hh"
#include "vkr/ui/ui.hh"
#include "vkr/util/timer.hh"
#include <memory>

namespace vkr::render {

struct UiPassDesc {
  RenderGraphPassDesc graph{
      .name = "ui", .reads = {"scene.color"}, .writes = {"swapchain"}};
  resource::SwapchainTargetDesc target{
      .depth = resource::DepthAttachmentDesc{.format = VK_FORMAT_D32_SFLOAT}};
  pipeline::DescriptorPoolDesc descriptorPool{
      .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
      .maxSets = core::MAX_FRAMES_IN_FLIGHT + 2};
  std::vector<VkClearValue> clearValues{
      VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
      VkClearValue{.depthStencil = {1.0f, 0}}};
};

class UiPass final : public RenderGraphPass {
public:
  UiPass(const core::Window &window, const core::Instance &instance,
         const core::Surface &surface, const core::Device &device,
         const core::CommandPool &commandPool, const core::Swapchain &swapchain,
         resource::ResourceManager &resourceManager, RasterPass &source,
         util::Timer &timer, ui::ThemeDesc &theme, UiPassDesc desc = {});
  ~UiPass() override;

  UiPass(const UiPass &) = delete;
  auto operator=(const UiPass &) -> UiPass & = delete;

  void create() override;
  void destroy() override;
  void update(const RenderGraphPassDesc &desc) override;
  void update(const UiPassDesc &desc);
  void record(Renderer &renderer) override;

  [[nodiscard]] auto ui() noexcept -> ui::UI * { return ui_.get(); }
  [[nodiscard]] auto ui() const noexcept -> const ui::UI * { return ui_.get(); }

  [[nodiscard]] auto shouldClose() const noexcept -> bool {
    return ui_ && ui_->shouldClose();
  }

  [[nodiscard]] auto layoutMode() const noexcept -> ui::LayoutMode {
    return ui_ ? ui_->layoutMode() : ui::LayoutMode::FullScreen;
  }

  [[nodiscard]] auto viewportInfo() const noexcept -> ui::ViewportInfo {
    return ui_ ? ui_->viewportInfo() : ui::ViewportInfo{};
  }

  void switchLayoutMode() {
    if (ui_) {
      ui_->switchLayoutMode();
    }
  }

private:
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Surface &surface_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const core::Swapchain &swapchain_;
  resource::ResourceManager &resource_manager_;
  RasterPass &source_;
  util::Timer &timer_;
  ui::ThemeDesc &theme_;

  UiPassDesc desc_{};
  std::unique_ptr<resource::SwapchainTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<resource::FramebufferSet> framebuffers_{};
  std::unique_ptr<pipeline::DescriptorPool> descriptor_pool_{};
  std::unique_ptr<ui::UI> ui_{};
};

class PresentPass final : public RenderGraphPass {
public:
  explicit PresentPass(RenderGraphPassDesc desc = {.name = "present",
                                                   .reads = {"swapchain"}}) {
    setDesc(std::move(desc));
  }

  void create() override {}
  void destroy() override {}
  void update(const RenderGraphPassDesc &desc) override { setDesc(desc); }
  void record(Renderer &renderer) override;
};

} // namespace vkr::render
