#include "impl/vertex.hpp"
#include "impl/universal_buffer.hpp"

Vertex::Vertex(glm::vec2 pos, glm::vec3 color) : pos(pos), color(color) {}
Vertex::~Vertex() {}

VertexBuffer::VertexBuffer(const std::vector<Vertex> &vertices, VkDevice device,
                           VkPhysicalDevice physicalDevice,
                           VkCommandPool commandPool, VkQueue graphicsQueue)
    : vertices(vertices), device(device), physicalDevice(physicalDevice),
      commandPool(commandPool), graphicsQueue(graphicsQueue) {
  create();
}

VertexBuffer::VertexBuffer(const std::vector<Vertex> &vertices,
                           const VulkanContext &ctx)
    : VertexBuffer(vertices, ctx.device, ctx.physicalDevice, ctx.commandPool,
                   ctx.graphicsQueue) {}

VertexBuffer::~VertexBuffer() { destroy(); }

void VertexBuffer::create() {
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory, device, physicalDevice);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, memory,
               device, physicalDevice);

  copyBuffer(stagingBuffer, vertexBuffer, bufferSize, commandPool,
             graphicsQueue, device);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VertexBuffer::destroy() {
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(device, memory, nullptr);
  }
  if (vertexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, vertexBuffer, nullptr);
  }
  vertices.clear();
}

void VertexBuffer::update(const std::vector<Vertex> &newVertices) {
  vertices = newVertices;
  destroy();
  create();
}
