#include "vkr/interface/sync_objects.hpp"
#include "vkr/buffers/command.hpp"

namespace vkr {
SyncObjects::SyncObjects(VkDevice device, std::vector<VkImage> swapchainImages)
    : device(device), swapchainImages(swapchainImages) {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  renderFinishedSemaphores.resize(swapchainImages.size());

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render finished "
                               "semaphore for swap chain image!");
    }
  }
}

SyncObjects::SyncObjects(const VulkanContext &ctx)
    : SyncObjects(ctx.device, ctx.swapchainImages) {}

SyncObjects::~SyncObjects() {
  for (auto &semaphore : imageAvailableSemaphores) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device, semaphore, nullptr);
    }
  }
  imageAvailableSemaphores.clear();
  for (auto &fence : inFlightFences) {
    if (fence != VK_NULL_HANDLE) {
      vkDestroyFence(device, fence, nullptr);
    }
  }
  inFlightFences.clear();
  for (auto &semaphore : renderFinishedSemaphores) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device, semaphore, nullptr);
    }
  }
  renderFinishedSemaphores.clear();
}
} // namespace vkr
