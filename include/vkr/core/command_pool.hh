#pragma once

#include "./device.hh"
#include "./surface.hh"

namespace vkr::core {

class CommandPool {
public:
  explicit CommandPool(const Device &device, const Surface &surface);
  ~CommandPool();

  CommandPool(const CommandPool &) = delete;
  CommandPool &operator=(const CommandPool &) = delete;

  [[nodiscard]] VkCommandPool commandPool() const noexcept {
    return vk_command_pool_;
  }

private:
  // dependencies
  const Device &device_;
  const Surface &surface_;

  // components
  VkCommandPool vk_command_pool_{VK_NULL_HANDLE};
};
} // namespace vkr::core
