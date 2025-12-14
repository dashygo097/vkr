#pragma once

#include "../resources/buffers/index_buffer.hh"
#include "../resources/buffers/vertex_buffer.hh"

namespace vkr::geometry {
class Mesh {
public:
  Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
       VkCommandPool commandPool, VkQueue graphicsQueue);
  Mesh(const VulkanContext &ctx);
  ~Mesh() = default;

  Mesh(const std::vector<Vertex> &vertices,
       const std::vector<uint16_t> &indices, const VulkanContext &ctx)
      : Mesh(ctx) {
    load(vertices, indices);
  }
  Mesh(const std::vector<Vertex> &vertices,
       const std::vector<uint16_t> &indices) {
    load(vertices, indices);
  }
  explicit Mesh(const std::string &meshFilePath, const VulkanContext &ctx)
      : Mesh(ctx) {
    load(meshFilePath);
  }
  explicit Mesh(const std::string &meshFilePath) { load(meshFilePath); };

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

public:
  [[nodiscard]] std::shared_ptr<VertexBuffer> vertexBuffer() const {
    return _vertexBuffer;
  }
  [[nodiscard]] std::shared_ptr<IndexBuffer> indexBuffer() const {
    return _indexBuffer;
  }

  void load(const std::vector<Vertex> &vertices,
            const std::vector<uint16_t> &indices);
  void load(const std::string &meshFilePath);

  void update(const std::vector<Vertex> &vertices,
              const std::vector<uint16_t> &indices) {
    checkDataLoaded();
    _vertexBuffer->update(vertices);
    _indexBuffer->update(indices);
  }
  void update(const std::vector<Vertex> &vertices) {
    checkDataLoaded();
    _vertexBuffer->update(vertices);
  }
  void update(const std::vector<uint16_t> &indices) {
    checkDataLoaded();
    _indexBuffer->update(indices);
  }

private:
  // dependencies
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;

  // components
  std::shared_ptr<VertexBuffer> _vertexBuffer;
  std::shared_ptr<IndexBuffer> _indexBuffer;

  void checkVulkanContext() {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE ||
        commandPool == VK_NULL_HANDLE || graphicsQueue == VK_NULL_HANDLE) {
      throw std::runtime_error("Vulkan context is not initialized");
    }
  }
  void checkDataLoaded() {
    if (!_vertexBuffer || !_indexBuffer) {
      throw std::runtime_error("Vertex or index buffer is not initialized");
    }
  }
};
} // namespace vkr::geometry
