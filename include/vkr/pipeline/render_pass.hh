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
  auto operator=(const RenderPass &) -> RenderPass & = delete;

  [[nodiscard]] auto renderPass() const noexcept -> VkRenderPass {
    return vk_render_pass_;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapchain_;

  // components
  VkRenderPass vk_render_pass_{VK_NULL_HANDLE};
};
} // namespace vkr::pipeline
