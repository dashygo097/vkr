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

  void layoutMode(LayoutMode mode) noexcept { _layoutMode = mode; }
  void switchLayoutMode() noexcept {
    switch (_layoutMode) {
    case LayoutMode::FullScreen:
      _layoutMode = LayoutMode::Standard;
      break;
    case LayoutMode::Standard:
      _layoutMode = LayoutMode::FullScreen;
      break;
    }
  }
  void viewportInfo(const ViewportInfo &info) noexcept { _viewportInfo = info; }

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
  LayoutMode _layoutMode{LayoutMode::FullScreen};
  ViewportInfo _viewportInfo{};

  // helpers
  void renderDockspace();
  void renderMainViewport();
  void renderPerformancePanel();

  void setupDockingLayout();
};
} // namespace vkr::ui
