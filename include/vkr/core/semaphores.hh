#pragma once

#include "./device.hh"

namespace vkr::core {
class Semaphore {
public:
  explicit Semaphore(const Device &device, const VkImage &image);

  ~Semaphore();
  Semaphore(const Semaphore &) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

  [[nodiscard]] VkSemaphore semaphore() const noexcept { return _semaphore; }

private:
  // dependenciesi
  const Device &device;
  const VkImage &image;

  // components
  VkSemaphore _semaphore;
};
} // namespace vkr::core
