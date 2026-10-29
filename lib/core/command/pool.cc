#include "vkr/core/command/pool.hh"
#include "vkr/logger.hh"

namespace vkr::core {
CommandPool::CommandPool(const Device &device, const CommandPoolDesc &desc)
    : device_(device), desc_(desc) {
  VKR_CORE_INFO("Creating command pool...");

  if (!desc_.isValid()) {
    VKR_CORE_ERROR("Invalid command pool descriptor");
  }

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

auto CommandPool::allocate() const -> CommandBuffer {
  CommandBuffer buffer{device_, *this};

  VkCommandBufferAllocateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = vk_command_pool_;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = 1;
  const auto result = vkAllocateCommandBuffers(device_.device(), &info,
                                               &buffer.vk_command_buffer_);
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to allocate command buffer. VkResult: {}",
                   static_cast<int>(result));
  }
  return buffer;
}

auto CommandPool::allocate(uint32_t count) const -> std::vector<CommandBuffer> {
  if (count == 0) {
    VKR_CORE_ERROR("Command buffer count must be nonzero");
  }

  std::vector<CommandBuffer> buffers{};
  buffers.reserve(count);
  for (uint32_t index = 0; index < count; ++index) {
    buffers.emplace_back(device_, *this);
  }
  std::vector<VkCommandBuffer> handles(count, VK_NULL_HANDLE);

  VkCommandBufferAllocateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = vk_command_pool_;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = count;
  const auto result =
      vkAllocateCommandBuffers(device_.device(), &info, handles.data());
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to allocate command buffers. VkResult: {}",
                   static_cast<int>(result));
  }

  for (uint32_t index = 0; index < count; ++index) {
    buffers[index].vk_command_buffer_ = handles[index];
  }
  return buffers;
}

} // namespace vkr::core
