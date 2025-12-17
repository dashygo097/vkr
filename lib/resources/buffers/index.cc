#include "vkr/resources/buffers/buffer_utils.hh"
#include "vkr/resources/buffers/index_buffer.hh"

namespace vkr {
IndexBuffer::IndexBuffer(const Device &device, const CommandPool &commandPool,
                         const std::vector<uint16_t> &indices)
    : _indices(indices), device(device), commandPool(commandPool) {
  create();
}

IndexBuffer::~IndexBuffer() { destroy(); }

void IndexBuffer::create() {
  if (_indices.empty()) {
    return;
  }
  VkDeviceSize bufferSize = sizeof(_indices[0]) * _indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory, device.device(),
               device.physicalDevice());

  void *data;
  vkMapMemory(device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, _indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device.device(), stagingBufferMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _memory,
               device.device(), device.physicalDevice());

  copyBuffer(stagingBuffer, _indexBuffer, bufferSize, commandPool.commandPool(),
             device.graphicsQueue(), device.device());

  vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
  vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
}

void IndexBuffer::destroy() {
  if (_memory != VK_NULL_HANDLE) {
    vkFreeMemory(device.device(), _memory, nullptr);
  }
  if (_indexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device.device(), _indexBuffer, nullptr);
  }
  if (!_indices.empty()) {
    _indices.clear();
  }
}

void IndexBuffer::update(const std::vector<uint16_t> &newIndices) {
  destroy();

  _indices = newIndices;
  create();
}
} // namespace vkr
