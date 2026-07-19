#include "vkr/core/sync/fence.hh"
#include "vkr/logger.hh"
#include <utility>

namespace vkr::core {

Fence::Fence(const Device &device, FenceDesc desc)
    : device_(std::cref(device)), desc_(desc) {
  VkFenceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  createInfo.flags = desc_.signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

  if (vkCreateFence(device_.get().device(), &createInfo, nullptr, &vk_fence_) !=
      VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create fence");
  }
}

Fence::~Fence() { destroy(); }

Fence::Fence(Fence &&other) noexcept
    : device_(other.device_), desc_(other.desc_),
      vk_fence_(std::exchange(other.vk_fence_, VK_NULL_HANDLE)) {}

auto Fence::operator=(Fence &&other) noexcept -> Fence & {
  if (this == &other) {
    return *this;
  }

  destroy();
  device_ = other.device_;
  desc_ = other.desc_;
  vk_fence_ = std::exchange(other.vk_fence_, VK_NULL_HANDLE);
  return *this;
}

auto Fence::isSignaled() const -> bool {
  const VkResult result = vkGetFenceStatus(device_.get().device(), vk_fence_);
  if (result == VK_SUCCESS) {
    return true;
  }
  if (result == VK_NOT_READY) {
    return false;
  }

  VKR_CORE_ERROR("Failed to query fence status");
}

void Fence::wait(uint64_t timeout) const {
  if (vkWaitForFences(device_.get().device(), 1, &vk_fence_, VK_TRUE,
                      timeout) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to wait for fence");
  }
}

void Fence::reset() const {
  if (vkResetFences(device_.get().device(), 1, &vk_fence_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to reset fence");
  }
}

void Fence::destroy() noexcept {
  if (vk_fence_ != VK_NULL_HANDLE) {
    vkDestroyFence(device_.get().device(), vk_fence_, nullptr);
    vk_fence_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::core
