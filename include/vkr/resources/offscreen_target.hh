#pragma once
#include "../core/command_pool.hh"
#include "../core/device.hh"
#include <vulkan/vulkan.h>

namespace vkr::resource {
class OffscreenTarget {
public:
  OffscreenTarget(const core::Device &device,
                  const core::CommandPool &commandPool, VkExtent2D extent);
  ~OffscreenTarget();

  OffscreenTarget(const OffscreenTarget &) = delete;
  auto operator=(const OffscreenTarget &) -> OffscreenTarget & = delete;

  void resize(VkExtent2D newExtent);
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
  [[nodiscard]] auto depthView() const noexcept -> VkImageView {
    return depth_view_;
  }
  [[nodiscard]] auto sampler() const noexcept -> VkSampler { return sampler_; }
  [[nodiscard]] auto imguiDescriptorSet() const noexcept -> VkDescriptorSet {
    return imgui_ds_;
  }
  [[nodiscard]] auto extent2D() const noexcept -> VkExtent2D { return extent_; }

  void registerWithImGui(VkDescriptorPool descriptorPool);

  void requestResize(VkExtent2D newExtent) noexcept {
    pending_extent_ = newExtent;
    resize_pending_ = true;
  }

  auto flushPendingResize(VkDescriptorPool pool) -> bool {
    if (!resize_pending_) {
      return false;
    }
    resize_pending_ = false;
    resize(pending_extent_);
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
  VkExtent2D extent_{};

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
  VkExtent2D pending_extent_{};

  // helpers
  auto findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) -> uint32_t;
};

} // namespace vkr::resource
