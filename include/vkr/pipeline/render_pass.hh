#pragma once

#include "../core/device.hh"
#include "../core/swapchain.hh"

namespace vkr {
class RenderPass {
public:
  explicit RenderPass(const Device &device, const Swapchain &swapchain);
  ~RenderPass();

  RenderPass(const RenderPass &) = delete;
  RenderPass &operator=(const RenderPass &) = delete;

  [[nodiscard]] VkRenderPass renderPass() const noexcept { return _renderPass; }

private:
  // dependencies
  const Device &device;
  const Swapchain &swapchain;

  // components
  VkRenderPass _renderPass{VK_NULL_HANDLE};
};
} // namespace vkr
