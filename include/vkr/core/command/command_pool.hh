#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/surface.hh"

namespace vkr::core {

class CommandPool {
public:
  explicit CommandPool(const Surface &surface, const Device &device);
  ~CommandPool();

  CommandPool(const CommandPool &) = delete;
  auto operator=(const CommandPool &) -> CommandPool & = delete;

  [[nodiscard]] auto commandPool() const noexcept -> VkCommandPool {
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
