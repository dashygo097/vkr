#pragma once

#include <GLFW/glfw3.h>

#include "interface/image.hpp"

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface);

class SwapChain {
public:
  SwapChain(GLFWwindow *window, VkDevice device,
            VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
            uint32_t width, uint32_t height);
  ~SwapChain();

  VkSwapchainKHR getSwapchain() const;

private:
  // dependencies
  GLFWwindow *window;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkSurfaceKHR surface;
  Images images;

  // components
  VkSwapchainKHR swapChain;
  VkExtent2D extent;

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
};
