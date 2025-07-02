#include "impl/index.hpp"
#include "impl/universal_buffer.hpp"

IndexBuffer::IndexBuffer(const std::vector<uint16_t> &indices, VkDevice device,
                         VkPhysicalDevice physicalDevice,
                         VkCommandPool commandPool, VkQueue graphicsQueue)
    : indices(indices), device(device), physicalDevice(physicalDevice),
      commandPool(commandPool), graphicsQueue(graphicsQueue) {
  create();
}

IndexBuffer::IndexBuffer(const std::vector<uint16_t> &indices,
                         const VulkanContext &ctx)
    : IndexBuffer(indices, ctx.device, ctx.physicalDevice, ctx.commandPool,
                  ctx.graphicsQueue) {}

IndexBuffer::~IndexBuffer() { destroy(); }

void IndexBuffer::create() {
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory, device, physicalDevice);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, memory, device,
               physicalDevice);

  copyBuffer(stagingBuffer, indexBuffer, bufferSize, commandPool, graphicsQueue,
             device);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void IndexBuffer::destroy() {
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(device, memory, nullptr);
  }
  if (indexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, indexBuffer, nullptr);
  }
  indices.clear();
}

void IndexBuffer::update(const std::vector<uint16_t> &newIndices) {
  indices = newIndices;
  destroy();
  create();
}
