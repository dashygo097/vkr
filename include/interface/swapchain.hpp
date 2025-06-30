#pragma once

#include <vector>
#include <vulkan/vulkan.h>

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain {
public:
  SwapChain(VkDevice device, VkPhysicalDevice physicalDevice,
            VkSurfaceKHR surface, uint32_t width, uint32_t height);
  ~SwapChain();

  VkSwapchainKHR getSwapchain() const;

private:
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapChain;
  std::vector<VkImage> chainImages;
  VkFormat chainImageFormat;
  VkExtent2D extent;
};
