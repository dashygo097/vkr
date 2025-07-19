#pragma once

#include "../ctx.hpp"

namespace vkr {

class CommandPool {
public:
  CommandPool(VkPhysicalDevice physicalDevice, VkDevice device,
              VkSurfaceKHR surface);
  CommandPool(const VulkanContext &ctx);
  ~CommandPool();

  CommandPool(const CommandPool &) = delete;
  CommandPool &operator=(const CommandPool &) = delete;

  VkCommandPool getVkCommandPool() const { return commandPool; }

private:
  // dependencies
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkSurfaceKHR surface;

  // components
  VkCommandPool commandPool{VK_NULL_HANDLE};
};
} // namespace vkr
