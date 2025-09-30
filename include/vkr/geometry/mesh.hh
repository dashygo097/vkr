#pragma once

#include "../buffers/index.hh"
#include "../buffers/vertex.hh"

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

  void load(const std::vector<Vertex> &vertices,
            const std::vector<uint16_t> &indices);
  void load(const std::string &meshFilePath);

  void update(const std::vector<Vertex> &vertices,
              const std::vector<uint16_t> &indices) {
    checkDataLoaded();
    vertexBuffer->update(vertices);
    indexBuffer->update(indices);
  }
  void update(const std::vector<Vertex> &vertices) {
    checkDataLoaded();
    vertexBuffer->update(vertices);
  }
  void update(const std::vector<uint16_t> &indices) {
    checkDataLoaded();
    indexBuffer->update(indices);
  }

private:
  // dependencies
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;

  // components
  std::unique_ptr<VertexBuffer> vertexBuffer;
  std::unique_ptr<IndexBuffer> indexBuffer;

  void checkVulkanContext() {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE ||
        commandPool == VK_NULL_HANDLE || graphicsQueue == VK_NULL_HANDLE) {
      throw std::runtime_error("Vulkan context is not initialized");
    }
  }
  void checkDataLoaded() {
    if (!vertexBuffer || !indexBuffer) {
      throw std::runtime_error("Vertex or index buffer is not initialized");
    }
  }
};
} // namespace vkr::geometry
