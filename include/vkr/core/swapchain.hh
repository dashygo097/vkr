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

  [[nodiscard]] VkSwapchainKHR swapchain() const noexcept { return _swapchain; }
  [[nodiscard]] std::vector<VkImage> images() const noexcept { return _images; }
  [[nodiscard]] std::vector<VkImageView> imageViews() const noexcept {
    return _imageViews;
  }
  [[nodiscard]] VkFormat format() const noexcept { return _format; }
  [[nodiscard]] VkExtent2D extent2D() const noexcept { return _extent; }

  void create();
  void destroy();
  void recreate();

private:
  // dependencies
  const Window &window;
  const Device &device;
  const Surface &surface;

  // components
  VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
  std::vector<VkImage> _images{};
  std::vector<VkImageView> _imageViews{};
  VkFormat _format{VK_FORMAT_UNDEFINED};
  VkExtent2D _extent{};
};
} // namespace vkr::core
