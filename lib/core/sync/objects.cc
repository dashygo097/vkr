#include "vkr/core/sync/objects.hh"
#include "vkr/core/command/buffers.hh"
#include "vkr/logger.hh"

namespace vkr::core {

SyncObjects::SyncObjects(const Device &device, const Swapchain &swapchain)
    : device_(device), swapchain_(swapchain) {
  create();
}

SyncObjects::~SyncObjects() { destroy(); }

void SyncObjects::recreate() {
  destroy();
  create();
}

void SyncObjects::create() {
  uint32_t imageCount = 0;

  if (vkGetSwapchainImagesKHR(device_.device(), swapchain_.swapchain(),
                              &imageCount, nullptr) != VK_SUCCESS) {
    VKR_CORE_ERROR("failed to query swapchain image count for sync objects");
  }

  if (imageCount == 0) {
    VKR_CORE_ERROR("swapchain has no images for sync objects");
  }

  vk_image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
  vk_render_finished_semaphores_.resize(imageCount);
  vk_in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device_.device(), &semaphoreInfo, nullptr,
                          &vk_image_available_semaphores_[i]) != VK_SUCCESS) {
      VKR_CORE_ERROR("failed to create image available semaphore {}", i);
    }

    if (vkCreateFence(device_.device(), &fenceInfo, nullptr,
                      &vk_in_flight_fences_[i]) != VK_SUCCESS) {
      VKR_CORE_ERROR("failed to create in-flight fence {}", i);
    }
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    if (vkCreateSemaphore(device_.device(), &semaphoreInfo, nullptr,
                          &vk_render_finished_semaphores_[i]) != VK_SUCCESS) {
      VKR_CORE_ERROR("failed to create render finished semaphore {}", i);
    }
  }
}

void SyncObjects::destroy() {
  for (VkSemaphore semaphore : vk_render_finished_semaphores_) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_.device(), semaphore, nullptr);
    }
  }

  for (VkSemaphore semaphore : vk_image_available_semaphores_) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_.device(), semaphore, nullptr);
    }
  }

  for (VkFence fence : vk_in_flight_fences_) {
    if (fence != VK_NULL_HANDLE) {
      vkDestroyFence(device_.device(), fence, nullptr);
    }
  }

  vk_render_finished_semaphores_.clear();
  vk_image_available_semaphores_.clear();
  vk_in_flight_fences_.clear();
}

auto SyncObjects::imageAvailableSemaphore(uint32_t frameIndex) const
    -> VkSemaphore {
  if (frameIndex >= vk_image_available_semaphores_.size()) {
    VKR_CORE_ERROR("image available semaphore frame index {} out of range, "
                   "count {}",
                   frameIndex, vk_image_available_semaphores_.size());
  }

  return vk_image_available_semaphores_[frameIndex];
}

auto SyncObjects::renderFinishedSemaphore(uint32_t imageIndex) const
    -> VkSemaphore {
  if (imageIndex >= vk_render_finished_semaphores_.size()) {
    VKR_CORE_ERROR("render finished semaphore image index {} out of range, "
                   "count {}",
                   imageIndex, vk_render_finished_semaphores_.size());
  }

  return vk_render_finished_semaphores_[imageIndex];
}

auto SyncObjects::inFlightFence(uint32_t frameIndex) const -> VkFence {
  if (frameIndex >= vk_in_flight_fences_.size()) {
    VKR_CORE_ERROR("in-flight fence frame index {} out of range, count {}",
                   frameIndex, vk_in_flight_fences_.size());
  }

  return vk_in_flight_fences_[frameIndex];
}

void SyncObjects::waitForFrame(uint32_t frameIndex) const {
  VkFence fence = inFlightFence(frameIndex);

  vkWaitForFences(device_.device(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void SyncObjects::resetFrame(uint32_t frameIndex) const {
  VkFence fence = inFlightFence(frameIndex);

  vkResetFences(device_.device(), 1, &fence);
}

} // namespace vkr::core
