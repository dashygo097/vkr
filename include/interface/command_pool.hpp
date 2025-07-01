#pragma once

#include <vulkan/vulkan.h>

class CommandPool {
public:
  CommandPool(VkPhysicalDevice physicalDevice, VkDevice device,
              VkSurfaceKHR surface);
  ~CommandPool();

  VkCommandPool getCommandPool() const { return commandPool; }

private:
  // dependencies
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkSurfaceKHR surface;

  // components
  VkCommandPool commandPool{VK_NULL_HANDLE};
};
