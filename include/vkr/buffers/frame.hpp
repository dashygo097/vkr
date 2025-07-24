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

  [[nodiscard]] std::vector<VkFramebuffer> framebuffers() const noexcept {
    return _framebuffers;
  }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  std::vector<VkImageView> swapchainImageViews{VK_NULL_HANDLE};
  VkExtent2D swapchainExtent{};

  // components
  std::vector<VkFramebuffer> _framebuffers{};
  bool frameBufferResized{false};
};
} // namespace vkr
