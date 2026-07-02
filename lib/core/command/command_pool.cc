#include "vkr/core/command/command_buffer.hh"
#include "vkr/logger.hh"

namespace vkr::core {
CommandPool::CommandPool(const Surface &surface, const Device &device)
    : surface_(surface), device_(device) {
  VKR_CORE_INFO("Creating command pool...");

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = device_.graphicsFamily();

  if (vkCreateCommandPool(device_.device(), &poolInfo, nullptr,
                          &vk_command_pool_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create command pool!");
  }

  VKR_CORE_INFO("Command pool created successfully.");
}

CommandPool::~CommandPool() {
  if (vk_command_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_.device(), vk_command_pool_, nullptr);
    vk_command_pool_ = VK_NULL_HANDLE;
  }
}
} // namespace vkr::core
