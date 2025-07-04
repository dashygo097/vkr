#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>

#include "ctx.hpp"

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

  VkSwapchainKHR getVkSwapchain() const { return swapchain; }
  std::vector<VkImage> getVkImages() const { return images; }
  std::vector<VkImageView> getVkImageViews() const { return imageViews; }
  VkFormat getVkFormat() const { return format; }
  VkExtent2D getVkExtent2D() const { return extent; }

  void create();
  void destroy();
  void recreate();

private:
  // dependencies
  GLFWwindow *window;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkSurfaceKHR surface;

  // components
  VkSwapchainKHR swapchain{VK_NULL_HANDLE};
  std::vector<VkImage> images{};
  std::vector<VkImageView> imageViews{};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkExtent2D extent{};
};
