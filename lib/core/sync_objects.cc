#include "vkr/core/sync_objects.hh"
#include "vkr/core/command_buffer.hh"
#include "vkr/logger.hh"

namespace vkr::core {
SyncObjects::SyncObjects(const Device &device, const Swapchain &swapchain)
    : device_(device), swapchain_(swapchain) {
  VKR_CORE_INFO("Creating synchronization objects...");

  vk_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
  vk_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

  vk_render_finished_semaphores.resize(swapchain.images().size());

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr,
                          &vk_image_available_semaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device.device(), &fenceInfo, nullptr,
                      &vk_in_flight_fences[i]) != VK_SUCCESS) {
      VKR_CORE_ERROR("failed to create synchronization objects for a frame!");
    }
  }

  for (size_t i = 0; i < swapchain.images().size(); i++) {
    if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr,
                          &vk_render_finished_semaphores[i]) != VK_SUCCESS) {
      VKR_CORE_ERROR(
          "failed to create render finished semaphore for swap chain image!");
    }
  }

  VKR_CORE_INFO("Synchronization objects created successfully.");
}

SyncObjects::~SyncObjects() {
  for (auto &semaphore : vk_image_available_semaphores) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_.device(), semaphore, nullptr);
    }
  }
  vk_image_available_semaphores.clear();
  for (auto &fence : vk_in_flight_fences) {
    if (fence != VK_NULL_HANDLE) {
      vkDestroyFence(device_.device(), fence, nullptr);
    }
  }
  vk_in_flight_fences.clear();
  for (auto &semaphore : vk_render_finished_semaphores) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_.device(), semaphore, nullptr);
    }
  }
  vk_render_finished_semaphores.clear();
}
} // namespace vkr::core
