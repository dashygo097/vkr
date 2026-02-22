#pragma once

#include "../core/command_pool.hh"
#include "../core/device.hh"
#include "../core/instance.hh"
#include "../core/window.hh"
#include "../pipeline/descriptors/pool.hh"
#include "../pipeline/graphics_pipeline.hh"
#include "../pipeline/render_pass.hh"
#include "../timer.hh"
#include "./components/fps_panel.hh"
#include "./components/shader_editor.hh"

namespace vkr::ui {

enum LayoutMode {
  FullScreen,
  Standard,
};

struct ViewportInfo {
  float x, y;
  float width, height;
  bool isFocused;
  bool isHovered;
};

class UI {
public:
  UI(const core::Window &window, const core::Instance &instance,
     const core::Surface &surface, const core::Device &device,
     const core::CommandPool &commandPool,
     const pipeline::RenderPass &renderPass,
     const pipeline::DescriptorPool &descriptorPool,
     pipeline::GraphicsPipeline &graphicsPipeline, const Timer &timer,
     pipeline::PipelineMode mode);
  ~UI();

  UI(const UI &) = delete;
  UI &operator=(const UI &) = delete;

  void render(VkCommandBuffer commandBuffer);

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
  void viewportInfo(const ViewportInfo &info) noexcept {
    viewport_info_ = info;
  }

  [[nodiscard]] LayoutMode layoutMode() const noexcept { return layout_mode_; }
  [[nodiscard]] const ViewportInfo &viewportInfo() const noexcept {
    return viewport_info_;
  }

private:
  // dependencies
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Surface &surface_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const pipeline::RenderPass &render_pass_;
  const pipeline::DescriptorPool &descriptor_pool_;
  pipeline::GraphicsPipeline &graphics_pipeline_;
  const Timer &timer_;
  pipeline::PipelineMode mode_;

  // components
  std::unique_ptr<FPSPanel> fps_panel_;
  std::unique_ptr<ShaderEditor> shader_editor_;

  // state
  LayoutMode layout_mode_{LayoutMode::FullScreen};
  ViewportInfo viewport_info_{};

  // helpers
  void renderDockspace();
  void renderMainViewport();
  void renderPerformancePanel();
  void renderShaderEditor();

  void setupDockingLayout();
};
} // namespace vkr::ui
