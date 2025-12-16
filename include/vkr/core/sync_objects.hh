#pragma once

#include "./device.hh"

namespace vkr {

class SyncObjects {
public:
  SyncObjects(const Device &device,
              const std::vector<VkImage> &swapchainImages);
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
  const std::vector<VkImage> &swapchainImages;

  // components
  std::vector<VkSemaphore> _imageAvailableSemaphores{};
  std::vector<VkSemaphore> _renderFinishedSemaphores{};
  std::vector<VkFence> _inFlightFences{};
};
} // namespace vkr
