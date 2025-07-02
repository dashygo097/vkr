#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "ctx.hpp"

class Framebuffers {
public:
  Framebuffers(VkDevice device, VkRenderPass renderPass,
               std::vector<VkImageView> swapchainImageViews,
               VkExtent2D swapchainExtend);
  Framebuffers(const VulkanContext &ctx);
  ~Framebuffers();

  void create();
  void destroy();

  std::vector<VkFramebuffer> getVkFramebuffers() const { return framebuffers; }

private:
  // dependencies
  VkDevice device;
  VkRenderPass renderPass;
  std::vector<VkImageView> swapchainImageViews;
  VkExtent2D swapchainExtent;

  // components
  std::vector<VkFramebuffer> framebuffers{};
  bool frameBufferResized{false};
};
