#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/surface.hh"
#include "vkr/core/window.hh"

namespace vkr::core {

struct SwapchainDesc {
  VkPresentModeKHR presentMode{VK_PRESENT_MODE_IMMEDIATE_KHR};

  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats{};
  std::vector<VkPresentModeKHR> presentModes{};

  VkSurfaceFormatKHR surfaceFormat{};
  VkExtent2D extent{};
  uint32_t imageCount{0};

  [[nodiscard]] auto isValid() const noexcept -> bool { return true; }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("present", presentMode);
  }
};

void querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface,
                           SwapchainDesc &desc);

auto chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats)
    -> VkSurfaceFormatKHR;

auto chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes,
    VkPresentModeKHR requestedMode) -> VkPresentModeKHR;

auto chooseSwapExtent(GLFWwindow *window,
                      const VkSurfaceCapabilitiesKHR &capabilities)
    -> VkExtent2D;

class Swapchain {
public:
  explicit Swapchain(const Window &window, const Surface &surface,
                     const Device &device, SwapchainDesc &desc);
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
    return desc_.surfaceFormat.format;
  }

  [[nodiscard]] auto extent2D() const noexcept -> VkExtent2D {
    return desc_.extent;
  }

  [[nodiscard]] auto imageCount() const noexcept -> size_t {
    return desc_.imageCount;
  }

  [[nodiscard]] auto presentMode() const noexcept -> VkPresentModeKHR {
    return desc_.presentMode;
  }

private:
  // dependencies
  const Window &window_;
  const Surface &surface_;
  const Device &device_;

  // components
  SwapchainDesc &desc_;
  VkSwapchainKHR vk_swapchain_{VK_NULL_HANDLE};
};

} // namespace vkr::core
