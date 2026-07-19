#include "vkr/core/sync/fence.hh"
#include "vkr/logger.hh"
#include <utility>

namespace vkr::core {

Fence::Fence(const Device &device, bool signaled) : device_(device) {
  VkFenceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

  if (vkCreateFence(device_.device(), &createInfo, nullptr, &vk_fence_) !=
      VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create fence");
  }
}

Fence::~Fence() { destroy(); }

Fence::Fence(Fence &&other) noexcept
    : device_(other.device_),
      vk_fence_(std::exchange(other.vk_fence_, VK_NULL_HANDLE)) {}

auto Fence::isSignaled() const -> bool {
  const VkResult result = vkGetFenceStatus(device_.device(), vk_fence_);
  if (result == VK_SUCCESS) {
    return true;
  }
  if (result == VK_NOT_READY) {
    return false;
  }

  VKR_CORE_ERROR("Failed to query fence status");
}

void Fence::wait(uint64_t timeout) const {
  if (vkWaitForFences(device_.device(), 1, &vk_fence_, VK_TRUE, timeout) !=
      VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to wait for fence");
  }
}

void Fence::reset() const {
  if (vkResetFences(device_.device(), 1, &vk_fence_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to reset fence");
  }
}

void Fence::destroy() noexcept {
  if (vk_fence_ != VK_NULL_HANDLE) {
    vkDestroyFence(device_.device(), vk_fence_, nullptr);
    vk_fence_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::core
