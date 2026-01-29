#pragma once

#include "../core/command_pool.hh"
#include "../core/device.hh"
#include "../core/instance.hh"
#include "../core/window.hh"
#include "../pipeline/descriptors/pool.hh"
#include "../pipeline/render_pass.hh"
#include "./fps_panel.hh"

namespace vkr::ui {
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

  bool isVisible() const noexcept { return _visible; }

  void visible() { _visible = true; }
  void invisible() { _visible = false; }
  void toggleVisibility() { _visible = !_visible; }

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

  bool _visible{false};
};
} // namespace vkr::ui
