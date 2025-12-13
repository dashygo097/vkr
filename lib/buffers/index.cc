#include "vkr/buffers/index.hh"
#include "vkr/buffers/universal.hh"

namespace vkr {
IndexBuffer::IndexBuffer(const std::vector<uint16_t> &indices, VkDevice device,
                         VkPhysicalDevice physicalDevice,
                         VkCommandPool commandPool, VkQueue graphicsQueue)
    : _indices(indices), device(device), physicalDevice(physicalDevice),
      commandPool(commandPool), graphicsQueue(graphicsQueue) {
  create();
}

IndexBuffer::IndexBuffer(const std::vector<uint16_t> &indices,
                         const VulkanContext &ctx)
    : IndexBuffer(indices, ctx.device, ctx.physicalDevice, ctx.commandPool,
                  ctx.graphicsQueue) {}

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
               stagingBuffer, stagingBufferMemory, device, physicalDevice);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, _indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _memory,
               device, physicalDevice);

  copyBuffer(stagingBuffer, _indexBuffer, bufferSize, commandPool,
             graphicsQueue, device);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void IndexBuffer::destroy() {
  if (_memory != VK_NULL_HANDLE) {
    vkFreeMemory(device, _memory, nullptr);
  }
  if (_indexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, _indexBuffer, nullptr);
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
