#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats);
VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes);
VkExtent2D chooseSwapExtent(GLFWwindow *window,
                            const VkSurfaceCapabilitiesKHR &capabilities);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

class SwapChain {
public:
  SwapChain(GLFWwindow *window, VkDevice device,
            VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
  ~SwapChain();

  VkSwapchainKHR getSwapChain() const { return swapChain; }
  VkImage getImage(uint32_t index) const { return images.at(index); }
  VkImageView getImageView(uint32_t index) const {
    return imageViews.at(index);
  }
  VkFormat getFormat() const { return format; }
  VkExtent2D getExtent() const { return extent; }

private:
  // dependencies
  GLFWwindow *window;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkSurfaceKHR surface;

  // components
  VkSwapchainKHR swapChain;
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  VkFormat format;
  VkExtent2D extent;
};
