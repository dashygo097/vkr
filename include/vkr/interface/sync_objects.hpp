#pragma once

#include "../ctx.hpp"

namespace vkr {

class SyncObjects {
public:
  SyncObjects(VkDevice device, std::vector<VkImage> swapchainImages);
  SyncObjects(const VulkanContext &ctx);
  ~SyncObjects();

  SyncObjects(const SyncObjects &) = delete;
  SyncObjects &operator=(const SyncObjects &) = delete;

  std::vector<VkSemaphore> getImageAvailableSemaphores() const {
    return imageAvailableSemaphores;
  }
  std::vector<VkSemaphore> getRenderFinishedSemaphores() const {
    return renderFinishedSemaphores;
  }
  std::vector<VkFence> getInFlightFences() const { return inFlightFences; }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  std::vector<VkImage> swapchainImages{};

  // components
  std::vector<VkSemaphore> imageAvailableSemaphores{};
  std::vector<VkSemaphore> renderFinishedSemaphores{};
  std::vector<VkFence> inFlightFences{};
};
} // namespace vkr
