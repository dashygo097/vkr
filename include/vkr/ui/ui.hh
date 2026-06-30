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
  float x, y, width, height;
  bool isFocused;
  bool isHovered;
};

class UI {
public:
  UI(const core::Window &window, const core::Instance &instance,
     const core::Surface &surface, const core::Device &device,
     const core::CommandPool &commandPool,
     const resource::ResourceManager &resourceManager,
     resource::OffscreenTarget &offscreenTarget,
     const pipeline::RenderPass &renderPass,
     const pipeline::DescriptorPool &descriptorPool,
     pipeline::GraphicsPipeline &graphicsPipeline, pipeline::PipelineMode mode,
     util::Timer &timer);
  ~UI();

  UI(const UI &) = delete;
  auto operator=(const UI &) -> UI & = delete;

  void render(VkCommandBuffer commandBuffer);

  void theme(const ThemeConfig &config) noexcept {
    theme_config_ = config;
    Theme::apply(theme_config_);
  }
  [[nodiscard]] auto theme() const noexcept -> const ThemeConfig & {
    return theme_config_;
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

  [[nodiscard]] auto shouldClose() -> bool { return should_close_; }

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
  // dependencies
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Surface &surface_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const resource::ResourceManager &resource_manager_;
  resource::OffscreenTarget &offscreen_target_;
  const pipeline::RenderPass &render_pass_;
  const pipeline::DescriptorPool &descriptor_pool_;
  pipeline::GraphicsPipeline &graphics_pipeline_;
  pipeline::PipelineMode mode_;
  util::Timer &timer_;

  // components
  std::unique_ptr<ResourceTree> resource_tree_;
  std::unique_ptr<FPSPanel> fps_panel_;
  std::unique_ptr<ShaderEditor> shader_editor_;
  std::unique_ptr<LoggingPanel> logging_panel_;

  // state
  ThemeConfig theme_config_{};
  LayoutMode layout_mode_{LayoutMode::FullScreen};
  ViewportInfo viewport_info_{};
  bool should_close_{false};

  // helpers
  // Full screen mode
  void renderFullScreen();

  // Standard mode
  void renderDockspace();
  void setupDockingLayout();
  void renderMainViewport();
  void renderResourcePanel();
  void renderLoggingPanel();
  void renderPerformancePanel();
  void renderShaderEditor();
};
} // namespace vkr::ui
