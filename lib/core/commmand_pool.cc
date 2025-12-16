#include "vkr/core/command_pool.hh"
#include "vkr/core/core_utils.hh"

namespace vkr {
CommandPool::CommandPool(const Device &device, const Surface &surface)
    : device(device), surface(surface) {

  QueueFamilyIndices queueFamilyIndices =
      findQueueFamilies(device.physicalDevice(), surface.surface());

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(device.device(), &poolInfo, nullptr, &_commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

CommandPool::~CommandPool() {
  if (_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device.device(), _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;
  }
}
} // namespace vkr
