#pragma once

#include "../../ctx.hh"

namespace vkr {

class Vertex {
public:
  Vertex(glm::vec3 pos, glm::vec3 color);
  ~Vertex();
  glm::vec3 pos;
  glm::vec3 color;

  bool operator==(const Vertex &other) const {
    return pos == other.pos && color == other.color;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
  }
};

class VertexBuffer {
public:
  VertexBuffer(const std::vector<Vertex> &vertices, VkDevice device,
               VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
               VkQueue graphicsQueue);
  VertexBuffer(const std::vector<Vertex> &vertices, const VulkanContext &ctx);
  ~VertexBuffer();

  VertexBuffer(const VertexBuffer &) = delete;
  VertexBuffer &operator=(const VertexBuffer &) = delete;

  VertexBuffer(VertexBuffer &&other) noexcept;
  VertexBuffer &operator=(VertexBuffer &&other) noexcept;

  void create();
  void destroy();
  void update(const std::vector<Vertex> &newVertices);

  [[nodiscard]] std::vector<Vertex> vertices() const noexcept {
    return _vertices;
  }
  [[nodiscard]] VkBuffer buffer() const noexcept { return _vertexBuffer; }
  [[nodiscard]] VkDeviceMemory bufferMemory() const noexcept { return _memory; }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkCommandPool commandPool{VK_NULL_HANDLE};
  VkQueue graphicsQueue{VK_NULL_HANDLE};

  // components
  std::vector<Vertex> _vertices{};
  VkBuffer _vertexBuffer{VK_NULL_HANDLE};
  VkDeviceMemory _memory{VK_NULL_HANDLE};
};
} // namespace vkr

namespace std {
template <typename T, glm::qualifier Q> struct hash<glm::vec<2, T, Q>> {
  size_t operator()(const glm::vec<2, T, Q> &vec) const noexcept {
    size_t h1 = hash<T>{}(vec.x);
    size_t h2 = hash<T>{}(vec.y);
    return (h1 ^ (h2 << 1));
  }
};
template <typename T, glm::qualifier Q> struct hash<glm::vec<3, T, Q>> {
  size_t operator()(const glm::vec<3, T, Q> &vec) const noexcept {
    size_t h1 = hash<T>{}(vec.x);
    size_t h2 = hash<T>{}(vec.y);
    size_t h3 = hash<T>{}(vec.z);
    return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
  }
};

template <> struct hash<vkr::Vertex> {
  size_t operator()(vkr::Vertex const &vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1);
  }
};
} // namespace std
