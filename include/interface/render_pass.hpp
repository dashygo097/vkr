#pragma once

#include <vulkan/vulkan.h>

class RenderPass {
public:
  RenderPass(VkDevice device, VkFormat swapChainFormat);
  ~RenderPass();

  VkRenderPass getRenderPass() const { return renderPass; }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkFormat swapChainFormat{VK_FORMAT_UNDEFINED};

  // components
  VkRenderPass renderPass{VK_NULL_HANDLE};
};
