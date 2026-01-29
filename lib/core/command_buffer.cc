#include "vkr/core/command_buffer.hh"

namespace vkr::core {

CommandBuffers::CommandBuffers(const Device &device,
                               const CommandPool &commandPool)
    : device(device), commandPool(commandPool) {
  _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool.commandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               _commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

CommandBuffers::~CommandBuffers() {
  if (!_commandBuffers.empty()) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(),
                         static_cast<uint32_t>(_commandBuffers.size()),
                         _commandBuffers.data());
  }
  _commandBuffers.clear();
}

} // namespace vkr::core
