#pragma once

#include "../ctx.hpp"

namespace vkr {
class RenderPass {
public:
  RenderPass(VkDevice device, VkFormat swapchainFormat);
  RenderPass(const VulkanContext &ctx);
  ~RenderPass();

  RenderPass(const RenderPass &) = delete;
  RenderPass &operator=(const RenderPass &) = delete;

  [[nodiscard]] VkRenderPass renderPass() const noexcept { return _renderPass; }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkFormat swapchainImageFormat{VK_FORMAT_UNDEFINED};

  // components
  VkRenderPass _renderPass{VK_NULL_HANDLE};
};
} // namespace vkr
