#pragma once

#include "../core/device.hh"
#include "../core/swapchain.hh"

namespace vkr::pipeline {
class RenderPass {
public:
  explicit RenderPass(const core::Device &device,
                      const core::Swapchain &swapchain);
  ~RenderPass();

  RenderPass(const RenderPass &) = delete;
  RenderPass &operator=(const RenderPass &) = delete;

  [[nodiscard]] VkRenderPass renderPass() const noexcept { return _renderPass; }

private:
  // dependencies
  const core::Device &device;
  const core::Swapchain &swapchain;

  // components
  VkRenderPass _renderPass{VK_NULL_HANDLE};
};
} // namespace vkr::pipeline
