#pragma once

#include <vulkan/vulkan.h>

#include "ctx.hpp"

class RenderPass {
public:
  RenderPass(VkDevice device, VkFormat swapchainFormat);
  RenderPass(const VulkanContext &ctx);
  ~RenderPass();

  VkRenderPass getRenderPass() const { return renderPass; }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkFormat swapchainImageFormat{VK_FORMAT_UNDEFINED};

  // components
  VkRenderPass renderPass{VK_NULL_HANDLE};
};
