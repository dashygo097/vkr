#pragma once

#include "../core/command_pool.hh"
#include "../core/device.hh"
#include "../core/instance.hh"
#include "../core/window.hh"
#include "../pipeline/descriptor/descriptor_pool.hh"
#include "../pipeline/render_pass.hh"
#include "./fps_panel.hh"

namespace vkr {
class UI {
public:
  UI(const Window &window, const Instance &instance, const Surface &surface,
     const Device &device, const RenderPass &renderPass,
     const DescriptorPool &descriptorPool, const CommandPool &commandPool);
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
  const Window &window;
  const Instance &instance;
  const Surface &surface;
  const Device &device;
  const RenderPass &renderPass;
  const DescriptorPool &descriptorPool;
  const CommandPool &commandPool;

  // components
  std::unique_ptr<FPSPanel> fps_panel;

  bool _visible{false};
};
} // namespace vkr
