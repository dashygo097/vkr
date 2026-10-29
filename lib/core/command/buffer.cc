#include "vkr/core/command/buffer.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/logger.hh"
#include <utility>

namespace vkr::core {

CommandBuffer::CommandBuffer(const Device &device,
                             const CommandPool &commandPool) noexcept
    : device_(device), command_pool_(commandPool) {}

CommandBuffer::~CommandBuffer() { destroy(); }

CommandBuffer::CommandBuffer(CommandBuffer &&other) noexcept
    : device_(other.device_), command_pool_(other.command_pool_),
      vk_command_buffer_(
          std::exchange(other.vk_command_buffer_, VK_NULL_HANDLE)) {}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags) {
  if (!valid()) {
    VKR_CORE_ERROR("Cannot begin an invalid command buffer");
  }
  VkCommandBufferBeginInfo info{};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.flags = flags;
  const auto result = vkBeginCommandBuffer(vk_command_buffer_, &info);
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to begin command buffer. VkResult: {}",
                   static_cast<int>(result));
  }
}

void CommandBuffer::end() {
  if (!valid()) {
    VKR_CORE_ERROR("Cannot end an invalid command buffer");
  }
  const auto result = vkEndCommandBuffer(vk_command_buffer_);
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to end command buffer. VkResult: {}",
                   static_cast<int>(result));
  }
}

void CommandBuffer::reset(VkCommandBufferResetFlags flags) {
  if (!valid()) {
    VKR_CORE_ERROR("Cannot reset an invalid command buffer");
  }
  if ((command_pool_.desc_.flags &
       VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) == 0) {
    VKR_CORE_ERROR(
        "Command buffer reset requires a pool supporting individual resets");
  }
  const auto result = vkResetCommandBuffer(vk_command_buffer_, flags);
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to reset command buffer. VkResult: {}",
                   static_cast<int>(result));
  }
}

void CommandBuffer::destroy() noexcept {
  if (valid()) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &vk_command_buffer_);
    vk_command_buffer_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::core
