#include "vkr/core/sync/semaphore.hh"
#include "vkr/logger.hh"
#include <utility>

namespace vkr::core {

Semaphore::Semaphore(const Device &device, SemaphoreDesc desc)
    : device_(std::cref(device)), desc_(desc) {
  VkSemaphoreCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore(device_.get().device(), &createInfo, nullptr,
                        &vk_semaphore_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create semaphore");
  }
}

Semaphore::~Semaphore() { destroy(); }

Semaphore::Semaphore(Semaphore &&other) noexcept
    : device_(other.device_), desc_(other.desc_),
      vk_semaphore_(std::exchange(other.vk_semaphore_, VK_NULL_HANDLE)) {}

auto Semaphore::operator=(Semaphore &&other) noexcept -> Semaphore & {
  if (this == &other) {
    return *this;
  }

  destroy();
  device_ = other.device_;
  desc_ = other.desc_;
  vk_semaphore_ = std::exchange(other.vk_semaphore_, VK_NULL_HANDLE);
  return *this;
}

void Semaphore::destroy() noexcept {
  if (vk_semaphore_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(device_.get().device(), vk_semaphore_, nullptr);
    vk_semaphore_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::core
