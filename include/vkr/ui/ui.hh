#pragma once

#include "../core/command_pool.hh"
#include "../core/device.hh"
#include "../core/instance.hh"
#include "../core/window.hh"
#include "../pipeline/descriptors/pool.hh"
#include "../pipeline/render_pass.hh"
#include "./fps_panel.hh"

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
     const pipeline::DescriptorPool &descriptorPool);
  ~UI();

  UI(const UI &) = delete;
  UI &operator=(const UI &) = delete;

  void render(VkCommandBuffer commandBuffer);

  void visible() noexcept { _visible = true; }
  void invisible() noexcept { _visible = false; }
  void toggleVisibility() noexcept { _visible = !_visible; }
  void layoutMode(LayoutMode mode) noexcept { _layoutMode = mode; }
  void viewportInfo(const ViewportInfo &info) noexcept { _viewportInfo = info; }

  [[nodiscard]] bool isVisible() const noexcept { return _visible; }
  [[nodiscard]] LayoutMode layoutMode() const noexcept { return _layoutMode; }
  [[nodiscard]] const ViewportInfo &viewportInfo() const noexcept {
    return _viewportInfo;
  }

private:
  // dependencies
  const core::Window &window;
  const core::Instance &instance;
  const core::Surface &surface;
  const core::Device &device;
  const core::CommandPool &commandPool;
  const pipeline::RenderPass &renderPass;
  const pipeline::DescriptorPool &descriptorPool;

  // components
  std::unique_ptr<FPSPanel> fps_panel;

  // state
  bool _visible{false};
  LayoutMode _layoutMode{LayoutMode::Standard};
  ViewportInfo _viewportInfo{};

  // helpers
  void renderDockspace();
  void renderMainViewport();
  void renderPerformancePanel();

  void setupDockingLayout();
  void updateViewportInfo();
};
} // namespace vkr::ui
