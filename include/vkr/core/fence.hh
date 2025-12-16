#pragma once

#include "../ctx.hh"

namespace vkr {
class Fence {
public:
  Fence(VkDevice device, VkImage image);
  Fence(const VulkanContext &ctx);
  ~Fence();

  Fence(const Fence &) = delete;
  Fence &operator=(const Fence &) = delete;

  [[nodiscard]] VkFence inFlightFences() const noexcept { return _fence; }

private:
  // dependencies
  VkDevice device;

  // components
  VkFence _fence;
};
} // namespace vkr
