#pragma once

#include "./device.hh"
#include "./swapchain.hh"

namespace vkr {

class SyncObjects {
public:
  explicit SyncObjects(const Device &device, const Swapchain &swapchain);
  ~SyncObjects();

  SyncObjects(const SyncObjects &) = delete;
  SyncObjects &operator=(const SyncObjects &) = delete;

  [[nodiscard]] std::vector<VkSemaphore>
  imageAvailableSemaphores() const noexcept {
    return _imageAvailableSemaphores;
  }
  [[nodiscard]] std::vector<VkSemaphore>
  renderFinishedSemaphores() const noexcept {
    return _renderFinishedSemaphores;
  }
  [[nodiscard]] std::vector<VkFence> inFlightFences() const noexcept {
    return _inFlightFences;
  }

private:
  // dependencies
  const Device &device;
  const Swapchain &swapchain;

  // components
  std::vector<VkSemaphore> _imageAvailableSemaphores{};
  std::vector<VkSemaphore> _renderFinishedSemaphores{};
  std::vector<VkFence> _inFlightFences{};
};
} // namespace vkr
