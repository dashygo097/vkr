#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/swapchain.hh"
#include <cstdint>

namespace vkr::exec {

class FrameSync {
public:
  explicit FrameSync(const core::Device &device,
                     const core::Swapchain &swapchain,
                     uint32_t framesInFlight);
  ~FrameSync();

  FrameSync(const FrameSync &) = delete;
  auto operator=(const FrameSync &) -> FrameSync & = delete;

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
  [[nodiscard]] auto framesInFlight() const noexcept -> uint32_t {
    return frames_in_flight_;
  }

  void waitForFrame(uint32_t frameIndex) const;
  void resetFrame(uint32_t frameIndex) const;
  void recreate();

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapchain_;
  uint32_t frames_in_flight_{0};

  // components
  std::vector<VkSemaphore> vk_image_available_semaphores_{};
  std::vector<VkSemaphore> vk_render_finished_semaphores_{};
  std::vector<VkFence> vk_in_flight_fences_{};

  void create();
  void destroy();
};

} // namespace vkr::exec
