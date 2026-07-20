#include "vkr/scene/geometry/index_buffer.hh"
#include "vkr/logger.hh"

namespace vkr::scene {

IndexBuffer::IndexBuffer(const core::Device &device,
                         const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      target_(std::make_unique<resource::Buffer>(device)) {}

IndexBuffer::~IndexBuffer() = default;

void IndexBuffer::create() {
  if (indices_.empty()) {
    VKR_RES_ERROR("Cannot create index buffer with no indices");
  }

  VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

  resource::Buffer staging{device_, bufferSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  staging.write(indices_.data(), bufferSize);

  target_->update(bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = command_pool_.commandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  if (vkAllocateCommandBuffers(device_.device(), &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to allocate index upload command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to begin index upload command buffer");
  }

  VkBufferCopy copyRegion{};
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, staging.buffer(), target_->buffer(), 1,
                  &copyRegion);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to end index upload command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(command_pool_.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to submit index upload command buffer");
  }

  vkQueueWaitIdle(command_pool_.queue());
  vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                       &commandBuffer);
}

void IndexBuffer::destroy() {
  target_->destroy();
  indices_.clear();
}

void IndexBuffer::update(const std::vector<uint16_t> &indices) {
  destroy();
  indices_ = indices;
  create();
}

} // namespace vkr::scene
