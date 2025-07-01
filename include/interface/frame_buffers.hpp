#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class FrameBuffers {
public:
  FrameBuffers(VkDevice device, VkRenderPass renderPass,
               std::vector<VkImageView> imageViews, VkExtent2D SwapExtend);
  ~FrameBuffers();

  void create();
  void destroy();

  std::vector<VkFramebuffer> getFrameBuffers() const { return frameBuffers; }

private:
  // dependencies
  VkDevice device;
  VkRenderPass renderPass;
  std::vector<VkImageView> swapChainImageViews;
  VkExtent2D swapChainExtent;

  // components
  std::vector<VkFramebuffer> frameBuffers{};
  bool frameBufferResized{false};
};
