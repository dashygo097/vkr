#pragma once

#include "vkr/core/device.hh"
#include <cstdint>
#include <functional>

namespace vkr::core {

struct FenceDesc {
  bool signaled{false};

  [[nodiscard]] auto isValid() const noexcept -> bool { return true; }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("signaled", signaled);
  }
};

class Fence {
public:
  explicit Fence(const Device &device, FenceDesc desc = {});
  ~Fence();

  Fence(const Fence &) = delete;
  auto operator=(const Fence &) -> Fence & = delete;

  Fence(Fence &&other) noexcept;
  auto operator=(Fence &&other) noexcept -> Fence &;

  [[nodiscard]] auto fence() const noexcept -> VkFence { return vk_fence_; }
  [[nodiscard]] auto isSignaled() const -> bool;

  void wait(uint64_t timeout = UINT64_MAX) const;
  void reset() const;

private:
  // dependencies
  std::reference_wrapper<const Device> device_;

  // components
  FenceDesc desc_{};
  VkFence vk_fence_{VK_NULL_HANDLE};

  void destroy() noexcept;
};

} // namespace vkr::core
