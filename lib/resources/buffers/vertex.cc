#include "vkr/resources/buffers/buffer_utils.hh"
#include "vkr/resources/buffers/vertex_buffer.hh"

namespace vkr {

Vertex::Vertex(glm::vec3 pos, glm::vec3 color) : pos(pos), color(color) {}
Vertex::~Vertex() {}

VertexBuffer::VertexBuffer(const Device &device, const CommandPool &commandPool,
                           const std::vector<Vertex> &vertices)
    : _vertices(vertices), device(device), commandPool(commandPool) {
  create();
}

VertexBuffer::~VertexBuffer() { destroy(); }

void VertexBuffer::create() {
  if (_vertices.empty()) {
    return;
  }
  VkDeviceSize bufferSize = sizeof(_vertices[0]) * _vertices.size();
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory, device.device(),
               device.physicalDevice());

  void *data;
  vkMapMemory(device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, _vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(device.device(), stagingBufferMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _memory,
               device.device(), device.physicalDevice());

  copyBuffer(stagingBuffer, _vertexBuffer, bufferSize,
             commandPool.commandPool(), device.graphicsQueue(),
             device.device());

  vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
  vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
}

void VertexBuffer::destroy() {
  if (_memory != VK_NULL_HANDLE) {
    vkFreeMemory(device.device(), _memory, nullptr);
  }
  if (_vertexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device.device(), _vertexBuffer, nullptr);
  }
  if (!_vertices.empty()) {
    _vertices.clear();
  }
}

void VertexBuffer::update(const std::vector<Vertex> &newVertices) {
  destroy();

  _vertices = newVertices;
  create();
}
} // namespace vkr
