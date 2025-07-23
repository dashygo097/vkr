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

  VkRenderPass getVkRenderPass() const { return renderPass; }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkFormat swapchainImageFormat{VK_FORMAT_UNDEFINED};

  // components
  VkRenderPass renderPass{VK_NULL_HANDLE};
};
} // namespace vkr
