#pragma once

#include "../ctx.hh"

namespace vkr {
class Semaphore {
public:
  explicit Semaphore(VkDevice device, VkImage images);
  Semaphore(const VulkanContext &ctx);

  ~Semaphore();
  Semaphore(const Semaphore &) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

  [[nodiscard]] VkSemaphore semaphore() const noexcept { return _semaphore; }

private:
  // dependenciesi
  VkDevice device;

  // components
  VkSemaphore _semaphore;
};
} // namespace vkr
