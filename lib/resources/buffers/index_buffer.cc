#include "vkr/resources/buffers/index_buffer.hh"
#include "vkr/resources/buffers/buffer_utils.hh"

namespace vkr::resource {
IndexBuffer::IndexBuffer(const core::Device &device,
                         const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  create();
}

IndexBuffer::~IndexBuffer() { destroy(); }

void IndexBuffer::create() {
  if (indices_.empty()) {
    return;
  }
  VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory, device_.device(),
               device_.physicalDevice());

  void *data;
  vkMapMemory(device_.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices_.data(), (size_t)bufferSize);
  vkUnmapMemory(device_.device(), stagingBufferMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_index_buffer_,
               vk_memory_, device_.device(), device_.physicalDevice());

  copyBuffer(stagingBuffer, vk_index_buffer_, bufferSize,
             command_pool_.commandPool(), device_.graphicsQueue(),
             device_.device());

  vkDestroyBuffer(device_.device(), stagingBuffer, nullptr);
  vkFreeMemory(device_.device(), stagingBufferMemory, nullptr);
}

void IndexBuffer::destroy() {
  if (vk_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_.device(), vk_memory_, nullptr);
  }
  if (vk_index_buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(device_.device(), vk_index_buffer_, nullptr);
  }
  if (!indices_.empty()) {
    indices_.clear();
  }
}

void IndexBuffer::update(const std::vector<uint16_t> &newIndices) {
  destroy();

  indices_ = newIndices;
  create();
}
} // namespace vkr::resource
