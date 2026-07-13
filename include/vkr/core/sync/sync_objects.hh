#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/swapchain.hh"

namespace vkr::core {

class SyncObjects {
public:
  explicit SyncObjects(const Device &device, const Swapchain &swapchain);
  ~SyncObjects();

  SyncObjects(const SyncObjects &) = delete;
  auto operator=(const SyncObjects &) -> SyncObjects & = delete;

  [[nodiscard]] auto imageAvailableSemaphores() const noexcept
      -> const std::vector<VkSemaphore> & {
    return vk_image_available_semaphores_;
  }

  [[nodiscard]] auto renderFinishedSemaphores() const noexcept
      -> const std::vector<VkSemaphore> & {
    return vk_render_finished_semaphores_;
  }

  [[nodiscard]] auto inFlightFences() const noexcept
      -> const std::vector<VkFence> & {
    return vk_in_flight_fences_;
  }

  [[nodiscard]] auto imageAvailableSemaphore(uint32_t frameIndex) const
      -> VkSemaphore;
  [[nodiscard]] auto renderFinishedSemaphore(uint32_t imageIndex) const
      -> VkSemaphore;
  [[nodiscard]] auto inFlightFence(uint32_t frameIndex) const -> VkFence;

  void waitForFrame(uint32_t frameIndex) const;
  void resetFrame(uint32_t frameIndex) const;
  void recreate();

private:
  // dependencies
  const Device &device_;
  const Swapchain &swapchain_;

  // components
  std::vector<VkSemaphore> vk_image_available_semaphores_{};
  std::vector<VkSemaphore> vk_render_finished_semaphores_{};
  std::vector<VkFence> vk_in_flight_fences_{};

  void create();
  void destroy();
};

} // namespace vkr::core
