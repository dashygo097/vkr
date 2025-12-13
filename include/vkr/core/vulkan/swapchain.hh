#pragma once

#include "../../ctx.hh"

namespace vkr {

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
  Swapchain(GLFWwindow *window, VkPhysicalDevice physicalDevice,
            VkDevice device, VkSurfaceKHR surface);
  Swapchain(const VulkanContext &ctx);
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
  GLFWwindow *window{nullptr};
  VkDevice device{VK_NULL_HANDLE};
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkSurfaceKHR surface{VK_NULL_HANDLE};

  // components
  VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
  std::vector<VkImage> _images{};
  std::vector<VkImageView> _imageViews{};
  VkFormat _format{VK_FORMAT_UNDEFINED};
  VkExtent2D _extent{};
};
} // namespace vkr
