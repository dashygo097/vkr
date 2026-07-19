#pragma once

#include "vkr/core/device.hh"
#include <functional>

namespace vkr::core {

struct SemaphoreDesc {
  [[nodiscard]] auto isValid() const noexcept -> bool { return true; }

  template <typename Archive> auto serialize(Archive &ar) -> void {}
};

class Semaphore {
public:
  explicit Semaphore(const Device &device, SemaphoreDesc desc = {});
  ~Semaphore();

  Semaphore(const Semaphore &) = delete;
  auto operator=(const Semaphore &) -> Semaphore & = delete;

  Semaphore(Semaphore &&other) noexcept;
  auto operator=(Semaphore &&other) noexcept -> Semaphore &;

  [[nodiscard]] auto semaphore() const noexcept -> VkSemaphore {
    return vk_semaphore_;
  }

private:
  // dependencies
  std::reference_wrapper<const Device> device_;

  // components
  SemaphoreDesc desc_{};
  VkSemaphore vk_semaphore_{VK_NULL_HANDLE};

  void destroy() noexcept;
};

} // namespace vkr::core
