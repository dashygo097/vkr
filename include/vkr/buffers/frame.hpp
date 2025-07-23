#pragma once

#include "../ctx.hpp"

namespace vkr {

class Framebuffers {
public:
  Framebuffers(VkDevice device, VkRenderPass renderPass,
               std::vector<VkImageView> swapchainImageViews,
               VkExtent2D swapchainExtend);
  Framebuffers(const VulkanContext &ctx);
  ~Framebuffers();

  Framebuffers(const Framebuffers &) = delete;
  Framebuffers &operator=(const Framebuffers &) = delete;

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
} // namespace vkr
