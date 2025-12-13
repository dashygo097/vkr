#include "vkr/core/vulkan/command_pool.hh"
#include "vkr/core/vulkan/utils.hh"

namespace vkr {
CommandPool::CommandPool(VkPhysicalDevice physicalDevice, VkDevice device,
                         VkSurfaceKHR surface)
    : physicalDevice(physicalDevice), device(device), surface(surface) {

  QueueFamilyIndices queueFamilyIndices =
      findQueueFamilies(physicalDevice, surface);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &_commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

CommandPool::CommandPool(const VulkanContext &context)
    : CommandPool(context.physicalDevice, context.device, context.surface) {}

CommandPool::~CommandPool() {
  if (_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;
  }
}
} // namespace vkr
