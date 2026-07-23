#include "vkr/core/command/buffers.hh"
#include "vkr/logger.hh"

namespace vkr::core {

CommandBuffers::CommandBuffers(const Device &device,
                               const CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {}

CommandBuffers::~CommandBuffers() { destroy(); }

void CommandBuffers::update(const CommandBuffersDesc &desc) {
  desc_ = desc;
  create();
}

void CommandBuffers::create() {
  destroy();

  if (!desc_.isValid()) {
    VKR_CORE_ERROR("CommandBuffersDesc is invalid");
  }

  VKR_CORE_INFO("Creating command buffers...");
  vk_command_buffers_.resize(desc_.size);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = command_pool_.commandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount =
      static_cast<uint32_t>(vk_command_buffers_.size());

  if (vkAllocateCommandBuffers(device_.device(), &allocInfo,
                               vk_command_buffers_.data()) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to allocate command buffers!");
  }

  VKR_CORE_INFO("Command buffers created successfully.");
}

void CommandBuffers::destroy() {
  if (!vk_command_buffers_.empty()) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(),
                         static_cast<uint32_t>(vk_command_buffers_.size()),
                         vk_command_buffers_.data());
  }
  vk_command_buffers_.clear();
}

} // namespace vkr::core
