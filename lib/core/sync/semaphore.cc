#include "vkr/core/sync/semaphore.hh"
#include "vkr/logger.hh"
#include <utility>

namespace vkr::core {

Semaphore::Semaphore(const Device &device) : device_(device) {
  VkSemaphoreCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore(device_.device(), &createInfo, nullptr,
                        &vk_semaphore_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create semaphore");
  }
}

Semaphore::~Semaphore() { destroy(); }

Semaphore::Semaphore(Semaphore &&other) noexcept
    : device_(other.device_),
      vk_semaphore_(std::exchange(other.vk_semaphore_, VK_NULL_HANDLE)) {}

void Semaphore::destroy() noexcept {
  if (vk_semaphore_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(device_.device(), vk_semaphore_, nullptr);
    vk_semaphore_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::core
