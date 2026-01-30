#pragma once

#include "./device.hh"

namespace vkr::core {
class Fence {
public:
  explicit Fence(const Device &device, const VkImage &image);
  ~Fence();

  Fence(const Fence &) = delete;
  Fence &operator=(const Fence &) = delete;

  [[nodiscard]] VkFence fence() const noexcept { return vk_fence_; }

private:
  // dependencies
  const Device &device_;
  const VkImage &image_;

  // components
  VkFence vk_fence_;
};
} // namespace vkr::core
