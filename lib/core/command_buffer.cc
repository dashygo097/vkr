#include "vkr/core/command_buffer.hh"

namespace vkr::core {

CommandBuffers::CommandBuffers(const Device &device,
                               const CommandPool &commandPool)
    : device_(device), commandPool_(commandPool) {
  vk_command_buffers_.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool.commandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount =
      static_cast<uint32_t>(vk_command_buffers_.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               vk_command_buffers_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

CommandBuffers::~CommandBuffers() {
  if (!vk_command_buffers_.empty()) {
    vkFreeCommandBuffers(device_.device(), commandPool_.commandPool(),
                         static_cast<uint32_t>(vk_command_buffers_.size()),
                         vk_command_buffers_.data());
  }
  vk_command_buffers_.clear();
}

} // namespace vkr::core
