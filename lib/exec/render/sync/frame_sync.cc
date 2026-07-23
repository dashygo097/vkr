#include "vkr/exec/render/sync/frame_sync.hh"
#include "vkr/logger.hh"

namespace vkr::exec {

FrameSync::FrameSync(const core::Device &device,
                     const core::Swapchain &swapchain,
                     uint32_t framesInFlight)
    : device_(device), swapchain_(swapchain),
      frames_in_flight_(framesInFlight) {
  if (frames_in_flight_ == 0) {
    VKR_EXEC_ERROR("FrameSync framesInFlight must be greater than zero");
  }

  create();
}

FrameSync::~FrameSync() { destroy(); }

void FrameSync::recreate() {
  destroy();
  create();
}

void FrameSync::create() {
  uint32_t imageCount = 0;

  if (vkGetSwapchainImagesKHR(device_.device(), swapchain_.swapchain(),
                              &imageCount, nullptr) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to query swapchain image count for sync objects");
  }

  if (imageCount == 0) {
    VKR_EXEC_ERROR("swapchain has no images for sync objects");
  }

  vk_image_available_semaphores_.resize(frames_in_flight_);
  vk_render_finished_semaphores_.resize(imageCount);
  vk_in_flight_fences_.resize(frames_in_flight_);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < frames_in_flight_; i++) {
    if (vkCreateSemaphore(device_.device(), &semaphoreInfo, nullptr,
                          &vk_image_available_semaphores_[i]) != VK_SUCCESS) {
      VKR_EXEC_ERROR("failed to create image available semaphore {}", i);
    }

    if (vkCreateFence(device_.device(), &fenceInfo, nullptr,
                      &vk_in_flight_fences_[i]) != VK_SUCCESS) {
      VKR_EXEC_ERROR("failed to create in-flight fence {}", i);
    }
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    if (vkCreateSemaphore(device_.device(), &semaphoreInfo, nullptr,
                          &vk_render_finished_semaphores_[i]) != VK_SUCCESS) {
      VKR_EXEC_ERROR("failed to create render finished semaphore {}", i);
    }
  }
}

void FrameSync::destroy() {
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

auto FrameSync::imageAvailableSemaphore(uint32_t frameIndex) const
    -> VkSemaphore {
  if (frameIndex >= vk_image_available_semaphores_.size()) {
    VKR_EXEC_ERROR("image available semaphore frame index {} out of range, "
                   "count {}",
                   frameIndex, vk_image_available_semaphores_.size());
  }

  return vk_image_available_semaphores_[frameIndex];
}

auto FrameSync::renderFinishedSemaphore(uint32_t imageIndex) const
    -> VkSemaphore {
  if (imageIndex >= vk_render_finished_semaphores_.size()) {
    VKR_EXEC_ERROR("render finished semaphore image index {} out of range, "
                   "count {}",
                   imageIndex, vk_render_finished_semaphores_.size());
  }

  return vk_render_finished_semaphores_[imageIndex];
}

auto FrameSync::inFlightFence(uint32_t frameIndex) const -> VkFence {
  if (frameIndex >= vk_in_flight_fences_.size()) {
    VKR_EXEC_ERROR("in-flight fence frame index {} out of range, count {}",
                   frameIndex, vk_in_flight_fences_.size());
  }

  return vk_in_flight_fences_[frameIndex];
}

void FrameSync::waitForFrame(uint32_t frameIndex) const {
  VkFence fence = inFlightFence(frameIndex);

  vkWaitForFences(device_.device(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void FrameSync::resetFrame(uint32_t frameIndex) const {
  VkFence fence = inFlightFence(frameIndex);

  vkResetFences(device_.device(), 1, &fence);
}

} // namespace vkr::exec
