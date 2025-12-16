#include "vkr/core/sync_objects.hh"
#include "vkr/core/command_buffer.hh"

namespace vkr {
SyncObjects::SyncObjects(const Device &device,
                         const std::vector<VkImage> &swapchainImages)
    : device(device), swapchainImages(swapchainImages) {
  _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  _renderFinishedSemaphores.resize(swapchainImages.size());

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr,
                          &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device.device(), &fenceInfo, nullptr,
                      &_inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr,
                          &_renderFinishedSemaphores[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render finished "
                               "semaphore for swap chain image!");
    }
  }
}

SyncObjects::~SyncObjects() {
  for (auto &semaphore : _imageAvailableSemaphores) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device.device(), semaphore, nullptr);
    }
  }
  _imageAvailableSemaphores.clear();
  for (auto &fence : _inFlightFences) {
    if (fence != VK_NULL_HANDLE) {
      vkDestroyFence(device.device(), fence, nullptr);
    }
  }
  _inFlightFences.clear();
  for (auto &semaphore : _renderFinishedSemaphores) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device.device(), semaphore, nullptr);
    }
  }
  _renderFinishedSemaphores.clear();
}
} // namespace vkr
