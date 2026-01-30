#pragma once

#include "./device.hh"

namespace vkr::core {
class Semaphore {
public:
  explicit Semaphore(const Device &device, const VkImage &image);

  ~Semaphore();
  Semaphore(const Semaphore &) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

  [[nodiscard]] VkSemaphore semaphore() const noexcept { return vk_semaphore_; }

private:
  // dependenciesi
  const Device &device_;
  const VkImage &image_;

  // components
  VkSemaphore vk_semaphore_;
};
} // namespace vkr::core
