#pragma once

#include "vkr/core/device.hh"

namespace vkr::core {

class Semaphore {
public:
  explicit Semaphore(const Device &device);
  ~Semaphore();

  Semaphore(const Semaphore &) = delete;
  auto operator=(const Semaphore &) -> Semaphore & = delete;

  Semaphore(Semaphore &&other) noexcept;
  auto operator=(Semaphore &&other) -> Semaphore & = delete;

  [[nodiscard]] auto semaphore() const noexcept -> VkSemaphore {
    return vk_semaphore_;
  }

private:
  // dependencies
  const Device &device_;

  // components
  VkSemaphore vk_semaphore_{VK_NULL_HANDLE};

  void destroy() noexcept;
};

} // namespace vkr::core
