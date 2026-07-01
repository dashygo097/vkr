#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/window.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/resource/targets/offscreen_target.hh"
#include "vkr/ui/components/fps_panel.hh"
#include "vkr/ui/components/logging_panel.hh"
#include "vkr/ui/components/resource_tree.hh"
#include "vkr/ui/components/shader_editor.hh"
#include "vkr/ui/theme.hh"
#include "vkr/util/timer.hh"
#include <memory>

namespace vkr::ui {

enum LayoutMode {
  FullScreen,
  Standard,
};

struct ViewportInfo {
  float x{0.0f};
  float y{0.0f};
  float width{0.0f};
  float height{0.0f};
  bool isFocused{false};
  bool isHovered{false};
};

class UI {
public:
  UI(const core::Window &window, const core::Instance &instance,
     const core::Surface &surface, const core::Device &device,
     const core::CommandPool &commandPool,
     const resource::ResourceManager &resourceManager,
     resource::OffscreenTarget &offscreenTarget,
     const pipeline::RenderPass &renderPass,
     const pipeline::DescriptorPool &descriptorPool, util::Timer &timer,
     ThemeDesc &desc);
  ~UI();

  UI(const UI &) = delete;
  auto operator=(const UI &) -> UI & = delete;

  void render(VkCommandBuffer commandBuffer);

  [[nodiscard]] auto desc() const noexcept -> const ThemeDesc & {
    return desc_;
  }

  void layoutMode(LayoutMode mode) noexcept { layout_mode_ = mode; }

  void switchLayoutMode() noexcept {
    switch (layout_mode_) {
    case LayoutMode::FullScreen:
      layout_mode_ = LayoutMode::Standard;
      break;
    case LayoutMode::Standard:
      layout_mode_ = LayoutMode::FullScreen;
      break;
    }
  }

  [[nodiscard]] auto shouldClose() const noexcept -> bool {
    return should_close_;
  }

  void viewportInfo(const ViewportInfo &info) noexcept {
    viewport_info_ = info;
  }

  [[nodiscard]] auto layoutMode() const noexcept -> LayoutMode {
    return layout_mode_;
  }

  [[nodiscard]] auto viewportInfo() const noexcept -> const ViewportInfo & {
    return viewport_info_;
  }

private:
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Surface &surface_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const resource::ResourceManager &resource_manager_;
  resource::OffscreenTarget &offscreen_target_;
  const pipeline::RenderPass &render_pass_;
  const pipeline::DescriptorPool &descriptor_pool_;
  util::Timer &timer_;

  ThemeDesc &desc_;
  std::unique_ptr<ResourceTree> resource_tree_;
  std::unique_ptr<FPSPanel> fps_panel_;
  std::unique_ptr<ShaderEditor> shader_editor_;
  std::unique_ptr<LoggingPanel> logging_panel_;

  LayoutMode layout_mode_{LayoutMode::FullScreen};
  ViewportInfo viewport_info_{};
  bool should_close_{false};

  void renderFullScreen();
  void renderDockspace();
  void setupDockingLayout();
  void renderMainViewport();
  void renderResourcePanel();
  void renderLoggingPanel();
  void renderPerformancePanel();
  void renderShaderEditor();
};

} // namespace vkr::ui
