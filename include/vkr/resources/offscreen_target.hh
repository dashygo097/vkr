#pragma once
#include "../core/command_pool.hh"
#include "../core/device.hh"
#include <vulkan/vulkan.h>

namespace vkr::resource {
class OffscreenTarget {
public:
  OffscreenTarget(const core::Device &device,
                  const core::CommandPool &commandPool, uint32_t width,
                  uint32_t height);
  ~OffscreenTarget();
  OffscreenTarget(const OffscreenTarget &) = delete;
  OffscreenTarget &operator=(const OffscreenTarget &) = delete;

  void resize(uint32_t width, uint32_t height);
  void create();
  void destroy();

  [[nodiscard]] VkRenderPass renderPass() const noexcept {
    return render_pass_;
  }
  [[nodiscard]] VkFramebuffer framebuffer() const noexcept {
    return framebuffer_;
  }
  [[nodiscard]] VkImageView colorView() const noexcept { return color_view_; }
  [[nodiscard]] VkSampler sampler() const noexcept { return sampler_; }
  [[nodiscard]] VkDescriptorSet imguiDescriptorSet() const noexcept {
    return imgui_ds_;
  }
  [[nodiscard]] uint32_t width() const noexcept { return width_; }
  [[nodiscard]] uint32_t height() const noexcept { return height_; }

  void registerWithImGui(VkDescriptorPool descriptorPool);

  void requestResize(uint32_t width, uint32_t height) noexcept {
    pending_width_ = width;
    pending_height_ = height;
    resize_pending_ = true;
  }

  bool flushPendingResize(VkDescriptorPool pool) {
    if (!resize_pending_)
      return false;
    resize_pending_ = false;
    resize(pending_width_, pending_height_);
    registerWithImGui(pool);
    return true;
  }

  [[nodiscard]] bool isResizePending() const noexcept {
    return resize_pending_;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  uint32_t width_, height_;

  // components
  VkImage color_image_{VK_NULL_HANDLE};
  VkDeviceMemory color_memory_{VK_NULL_HANDLE};
  VkImageView color_view_{VK_NULL_HANDLE};

  VkImage depth_image_{VK_NULL_HANDLE};
  VkDeviceMemory depth_memory_{VK_NULL_HANDLE};
  VkImageView depth_view_{VK_NULL_HANDLE};

  VkSampler sampler_{VK_NULL_HANDLE};
  VkRenderPass render_pass_{VK_NULL_HANDLE};
  VkFramebuffer framebuffer_{VK_NULL_HANDLE};
  VkDescriptorSet imgui_ds_{VK_NULL_HANDLE};

  // states
  bool resize_pending_{false};
  uint32_t pending_width_{0};
  uint32_t pending_height_{0};

  // helpers
  uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags props);
};

} // namespace vkr::resource
