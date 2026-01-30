#pragma once

#include "./device.hh"
#include "./swapchain.hh"

namespace vkr::core {

class SyncObjects {
public:
  explicit SyncObjects(const Device &device, const Swapchain &swapchain);
  ~SyncObjects();

  SyncObjects(const SyncObjects &) = delete;
  SyncObjects &operator=(const SyncObjects &) = delete;

  [[nodiscard]] std::vector<VkSemaphore>
  imageAvailableSemaphores() const noexcept {
    return vk_image_available_semaphores;
  }
  [[nodiscard]] std::vector<VkSemaphore>
  renderFinishedSemaphores() const noexcept {
    return vk_render_finished_semaphores;
  }
  [[nodiscard]] std::vector<VkFence> inFlightFences() const noexcept {
    return vk_in_flight_fences;
  }

private:
  // dependencies
  const Device &device_;
  const Swapchain &swapchain_;

  // components
  std::vector<VkSemaphore> vk_image_available_semaphores{};
  std::vector<VkSemaphore> vk_render_finished_semaphores{};
  std::vector<VkFence> vk_in_flight_fences{};
};
} // namespace vkr::core
