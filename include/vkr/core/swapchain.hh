#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/surface.hh"
#include "vkr/core/window.hh"
#include <cstddef>
#include <cstdint>

namespace vkr::core {

struct SwapchainDesc {
  VkPresentModeKHR presentMode{VK_PRESENT_MODE_IMMEDIATE_KHR};
  uint32_t minImageCount{0};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    switch (presentMode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
    case VK_PRESENT_MODE_MAILBOX_KHR:
    case VK_PRESENT_MODE_FIFO_KHR:
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
      return true;
    default:
      return false;
    }
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("present", presentMode);
    ar("minImageCount", minImageCount);
  }
};

class Swapchain {
public:
  explicit Swapchain(const Window &window, const Surface &surface,
                     const Device &device, const SwapchainDesc &desc);
  ~Swapchain();

  Swapchain(const Swapchain &) = delete;
  auto operator=(const Swapchain &) -> Swapchain & = delete;

  [[nodiscard]] auto desc() const noexcept -> const SwapchainDesc & {
    return desc_;
  }

  [[nodiscard]] auto swapchain() const noexcept -> VkSwapchainKHR {
    return vk_swapchain_;
  }

  [[nodiscard]] auto format() const noexcept -> VkFormat {
    return surface_format_.format;
  }

  [[nodiscard]] auto width() const noexcept -> uint32_t {
    return extent_.width;
  }

  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return extent_.height;
  }

  [[nodiscard]] auto extent2D() const noexcept -> VkExtent2D { return extent_; }

  [[nodiscard]] auto imageCount() const noexcept -> size_t {
    return image_count_;
  }

  [[nodiscard]] auto presentMode() const noexcept -> VkPresentModeKHR {
    return present_mode_;
  }

  void update(const SwapchainDesc &desc);
  void recreate();

private:
  // dependencies
  const Window &window_;
  const Surface &surface_;
  const Device &device_;

  // components
  SwapchainDesc desc_{};
  VkSwapchainKHR vk_swapchain_{VK_NULL_HANDLE};

  // state
  VkSurfaceFormatKHR surface_format_{};
  VkExtent2D extent_{};
  uint32_t image_count_{0};
  VkPresentModeKHR present_mode_{VK_PRESENT_MODE_MAX_ENUM_KHR};

  void create();
  void destroy();
};

} // namespace vkr::core
