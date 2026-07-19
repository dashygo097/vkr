#pragma once

#include "vkr/core/device.hh"
#include <cstdint>

namespace vkr::core {

class Fence {
public:
  explicit Fence(const Device &device, bool signaled = false);
  ~Fence();

  Fence(const Fence &) = delete;
  auto operator=(const Fence &) -> Fence & = delete;

  Fence(Fence &&other) noexcept;
  auto operator=(Fence &&other) -> Fence & = delete;

  [[nodiscard]] auto fence() const noexcept -> VkFence { return vk_fence_; }
  [[nodiscard]] auto isSignaled() const -> bool;

  void wait(uint64_t timeout = UINT64_MAX) const;
  void reset() const;

private:
  // dependencies
  const Device &device_;

  // components
  VkFence vk_fence_{VK_NULL_HANDLE};

  void destroy() noexcept;
};

} // namespace vkr::core
