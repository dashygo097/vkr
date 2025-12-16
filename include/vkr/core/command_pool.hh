#pragma once

#include "./device.hh"
#include "surface.hh"

namespace vkr {

class CommandPool {
public:
  explicit CommandPool(const Device &device, const Surface &surface);
  ~CommandPool();

  CommandPool(const CommandPool &) = delete;
  CommandPool &operator=(const CommandPool &) = delete;

  [[nodiscard]] VkCommandPool commandPool() const noexcept {
    return _commandPool;
  }

private:
  // dependencies
  const Device &device;
  const Surface &surface;

  // components
  VkCommandPool _commandPool{VK_NULL_HANDLE};
};
} // namespace vkr
