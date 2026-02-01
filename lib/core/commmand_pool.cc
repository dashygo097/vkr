#include "vkr/core/command_pool.hh"
#include "vkr/core/queue_families.hh"

namespace vkr::core {
CommandPool::CommandPool(const Device &device, const Surface &surface)
    : device_(device), surface_(surface) {

  QueueFamilyIndices queueFamilyIndices(surface, device.physicalDevice());

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily();

  if (vkCreateCommandPool(device.device(), &poolInfo, nullptr,
                          &vk_command_pool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

CommandPool::~CommandPool() {
  if (vk_command_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_.device(), vk_command_pool_, nullptr);
    vk_command_pool_ = VK_NULL_HANDLE;
  }
}
} // namespace vkr::core
