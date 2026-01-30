#pragma once

#include "./device.hh"
#include "./window.hh"

namespace vkr::core {

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats);
VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes);
VkExtent2D chooseSwapExtent(GLFWwindow *window,
                            const VkSurfaceCapabilitiesKHR &capabilities);

class Swapchain {
public:
  explicit Swapchain(const Window &window, const Device &device,
                     const Surface &surface);
  ~Swapchain();

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  [[nodiscard]] VkSwapchainKHR swapchain() const noexcept {
    return vk_swapchain_;
  }
  [[nodiscard]] std::vector<VkImage> images() const noexcept {
    return vk_images_;
  }
  [[nodiscard]] std::vector<VkImageView> imageViews() const noexcept {
    return vk_imageviews_;
  }
  [[nodiscard]] VkFormat format() const noexcept { return vk_format_; }
  [[nodiscard]] VkExtent2D extent2D() const noexcept { return vk_extent_; }

  void create();
  void destroy();
  void recreate();

private:
  // dependencies
  const Window &window_;
  const Device &device_;
  const Surface &surface_;

  // components
  VkSwapchainKHR vk_swapchain_{VK_NULL_HANDLE};
  std::vector<VkImage> vk_images_{};
  std::vector<VkImageView> vk_imageviews_{};
  VkFormat vk_format_{VK_FORMAT_UNDEFINED};
  VkExtent2D vk_extent_{};
};
} // namespace vkr::core
