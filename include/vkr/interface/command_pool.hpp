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

  [[nodiscard]] VkCommandPool commandPool() const noexcept {
    return _commandPool;
  }

private:
  // dependencies
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkDevice device{VK_NULL_HANDLE};
  VkSurfaceKHR surface{VK_NULL_HANDLE};

  // components
  VkCommandPool _commandPool{VK_NULL_HANDLE};
};
} // namespace vkr
