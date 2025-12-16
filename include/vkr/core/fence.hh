#pragma once

#include "./device.hh"

namespace vkr {
class Fence {
public:
  explicit Fence(const Device &device, const VkImage &image);
  ~Fence();

  Fence(const Fence &) = delete;
  Fence &operator=(const Fence &) = delete;

  [[nodiscard]] VkFence fence() const noexcept { return _fence; }

private:
  // dependencies
  const Device &device;
  const VkImage &image;

  // components
  VkFence _fence;
};
} // namespace vkr
