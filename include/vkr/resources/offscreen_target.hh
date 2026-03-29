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
  auto operator=(const OffscreenTarget &) -> OffscreenTarget & = delete;

  void resize(uint32_t width, uint32_t height);
  void create();
  void destroy();

  [[nodiscard]] auto renderPass() const noexcept -> VkRenderPass {
    return render_pass_;
  }
  [[nodiscard]] auto framebuffer() const noexcept -> VkFramebuffer {
    return framebuffer_;
  }
  [[nodiscard]] auto colorView() const noexcept -> VkImageView {
    return color_view_;
  }
  [[nodiscard]] auto sampler() const noexcept -> VkSampler { return sampler_; }
  [[nodiscard]] auto imguiDescriptorSet() const noexcept -> VkDescriptorSet {
    return imgui_ds_;
  }
  [[nodiscard]] auto width() const noexcept -> uint32_t { return width_; }
  [[nodiscard]] auto height() const noexcept -> uint32_t { return height_; }

  void registerWithImGui(VkDescriptorPool descriptorPool);

  void requestResize(uint32_t width, uint32_t height) noexcept {
    pending_width_ = width;
    pending_height_ = height;
    resize_pending_ = true;
  }

  auto flushPendingResize(VkDescriptorPool pool) -> bool {
    if (!resize_pending_) {
      return false;
    }
    resize_pending_ = false;
    resize(pending_width_, pending_height_);
    registerWithImGui(pool);
    return true;
  }

  [[nodiscard]] auto isResizePending() const noexcept -> bool {
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
  auto findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) -> uint32_t;
};

} // namespace vkr::resource
