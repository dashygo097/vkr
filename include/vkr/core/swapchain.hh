#pragma once

#include "./device.hh"
#include "./window.hh"

namespace vkr::core {

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

auto querySwapchainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) -> SwapchainSupportDetails;
auto chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) -> VkSurfaceFormatKHR;
auto chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) -> VkPresentModeKHR;
auto chooseSwapExtent(GLFWwindow *window,
                            const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D;

class Swapchain {
public:
  explicit Swapchain(const Window &window, const Device &device,
                     const Surface &surface);
  ~Swapchain();

  Swapchain(const Swapchain &) = delete;
  auto operator=(const Swapchain &) -> Swapchain & = delete;

  [[nodiscard]] auto swapchain() const noexcept -> VkSwapchainKHR {
    return vk_swapchain_;
  }
  [[nodiscard]] auto images() const noexcept -> std::vector<VkImage> {
    return vk_images_;
  }
  [[nodiscard]] auto imageViews() const noexcept -> std::vector<VkImageView> {
    return vk_imageviews_;
  }
  [[nodiscard]] auto format() const noexcept -> VkFormat { return vk_format_; }
  [[nodiscard]] auto extent2D() const noexcept -> VkExtent2D { return vk_extent_; }

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
