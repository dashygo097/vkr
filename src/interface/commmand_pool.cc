#include "interface/command_pool.hpp"
#include "interface/vk_utils.hpp"

CommandPool::CommandPool(VkPhysicalDevice physicalDevice, VkDevice device,
                         VkSurfaceKHR surface)
    : physicalDevice(physicalDevice), device(device), surface(surface) {

  QueueFamilyIndices queueFamilyIndices =
      findQueueFamilies(physicalDevice, surface);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

CommandPool::CommandPool(const VulkanContext &context)
    : CommandPool(context.physicalDevice, context.device, context.surface) {}

CommandPool::~CommandPool() {
  if (commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, commandPool, nullptr);
    commandPool = VK_NULL_HANDLE;
  }
}
