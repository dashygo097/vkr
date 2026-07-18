#include "vkr/core/command/pool.hh"
#include "vkr/logger.hh"

namespace vkr::core {
CommandPool::CommandPool(const Device &device, CommandPoolDesc &desc)
    : device_(device), desc_(desc) {
  VKR_CORE_INFO("Creating command pool...");

  resolveQueue();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = desc_.flags;
  poolInfo.queueFamilyIndex = queue_family_;

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

void CommandPool::resolveQueue() {
  switch (desc_.queueRole) {
  case CommandQueueRole::Graphics:
    if (!device_.supportsGraphics()) {
      VKR_CORE_ERROR("Cannot create graphics command pool: unsupported queue");
    }
    queue_family_ = device_.graphicsFamily();
    queue_ = device_.graphicsQueue();
    break;
  case CommandQueueRole::Compute:
    if (!device_.supportsCompute()) {
      VKR_CORE_ERROR("Cannot create compute command pool: unsupported queue");
    }
    queue_family_ = device_.computeFamily();
    queue_ = device_.computeQueue();
    break;
  case CommandQueueRole::Transfer:
    if (!device_.supportsTransfer()) {
      VKR_CORE_ERROR("Cannot create transfer command pool: unsupported queue");
    }
    queue_family_ = device_.transferFamily();
    queue_ = device_.transferQueue();
    break;
  }
}
} // namespace vkr::core
