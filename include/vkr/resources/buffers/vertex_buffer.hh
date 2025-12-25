#pragma once

#include "../../core/command_pool.hh"
#include "../../core/device.hh"
#include "./buffer_utils.hh"
#include <glm/glm.hpp>

namespace vkr {

class IVertexBuffer {
public:
  virtual ~IVertexBuffer() = default;

  virtual VkBuffer buffer() const noexcept = 0;
  virtual VkDeviceMemory bufferMemory() const noexcept = 0;
  virtual size_t vertexCount() const noexcept = 0;

  virtual void updateRaw(const void *data, size_t count) = 0;
};

template <typename VertexType> class VertexBufferBase : public IVertexBuffer {
public:
  explicit VertexBufferBase(const Device &device,
                            const CommandPool &commandPool)
      : device(device), commandPool(commandPool) {
    create();
  }

  virtual ~VertexBufferBase() { destroy(); }

  VertexBufferBase(const VertexBufferBase &) = delete;
  VertexBufferBase &operator=(const VertexBufferBase &) = delete;

  void update(const std::vector<VertexType> &newVertices) {
    if (newVertices.empty()) {
      throw std::runtime_error("Cannot update vertex buffer with no vertices");
    }

    VkDeviceSize newBufferSize = sizeof(VertexType) * newVertices.size();
    VkDeviceSize oldBufferSize = sizeof(VertexType) * _vertices.size();

    // If size changed, recreate the buffer
    if (newBufferSize != oldBufferSize) {
      destroy();
      _vertices = newVertices;
      create();
      return;
    }

    // Size is the same, just update the data
    _vertices = newVertices;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(newBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory, device.device(),
                 device.physicalDevice());

    // Copy new vertex data to staging buffer
    void *data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, newBufferSize, 0,
                &data);
    memcpy(data, _vertices.data(), static_cast<size_t>(newBufferSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    // Copy from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, _vertexBuffer, newBufferSize,
               commandPool.commandPool(), device.graphicsQueue(),
               device.device());

    // Clean up staging buffer
    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
  }

  void updateRaw(const void *data, size_t count) override {
    const VertexType *vertices = static_cast<const VertexType *>(data);
    std::vector<VertexType> newVertices(vertices, vertices + count);
    update(newVertices);
  }

  [[nodiscard]] VkBuffer buffer() const noexcept override {
    return _vertexBuffer;
  }

  [[nodiscard]] VkDeviceMemory bufferMemory() const noexcept override {
    return _memory;
  }

  [[nodiscard]] size_t vertexCount() const noexcept override {
    return _vertices.size();
  }

  [[nodiscard]] const std::vector<VertexType> &vertices() const noexcept {
    return _vertices;
  }

protected:
  void create() {
    if (_vertices.empty()) {
      return;
    }

    VkDeviceSize bufferSize = sizeof(VertexType) * _vertices.size();

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory, device.device(),
                 device.physicalDevice());

    // Copy vertex data to staging buffer
    void *data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, _vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    // Create vertex buffer in device local memory
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _memory,
                 device.device(), device.physicalDevice());

    // Copy from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, _vertexBuffer, bufferSize,
               commandPool.commandPool(), device.graphicsQueue(),
               device.device());

    // Clean up staging buffer
    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
  }

  void destroy() {
    if (_memory != VK_NULL_HANDLE) {
      vkFreeMemory(device.device(), _memory, nullptr);
      _memory = VK_NULL_HANDLE;
    }
    if (_vertexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device.device(), _vertexBuffer, nullptr);
      _vertexBuffer = VK_NULL_HANDLE;
    }
  }

protected:
  // dependencies
  const Device &device;
  const CommandPool &commandPool;

  // components
  std::vector<VertexType> _vertices{};
  VkBuffer _vertexBuffer{VK_NULL_HANDLE};
  VkDeviceMemory _memory{VK_NULL_HANDLE};
};

struct Vertex3D {
public:
  glm::vec3 pos;
  glm::vec3 color;

  bool operator==(const Vertex3D &other) const {
    return pos == other.pos && color == other.color;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex3D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex3D, color);

    return attributeDescriptions;
  }
};

struct VertexNormal3D {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;

  bool operator==(const VertexNormal3D &other) const {
    return pos == other.pos && color == other.color && normal == other.normal;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(VertexNormal3D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(VertexNormal3D, pos);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(VertexNormal3D, color);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(VertexNormal3D, normal);
    return attributeDescriptions;
  }
};

struct VertexTextured3D {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  bool operator==(const VertexTextured3D &other) const {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(VertexTextured3D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(VertexTextured3D, pos);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(VertexTextured3D, color);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(VertexTextured3D, texCoord);
    return attributeDescriptions;
  }
};

struct VertexNormalTexture3D {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 texCoord;

  bool operator==(const VertexNormalTexture3D &other) const {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord && normal == other.normal;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(VertexNormalTexture3D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 4>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(VertexNormalTexture3D, pos);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(VertexNormalTexture3D, color);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(VertexNormalTexture3D, normal);
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(VertexNormalTexture3D, texCoord);
    return attributeDescriptions;
  }
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

template <> struct hash<vkr::Vertex3D> {
  size_t operator()(vkr::Vertex3D const &vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1);
  }
};

template <> struct hash<vkr::VertexNormal3D> {
  size_t operator()(vkr::VertexNormal3D const &vertex) const {
    return (((hash<glm::vec3>()(vertex.pos) ^
              (hash<glm::vec3>()(vertex.color) << 1)) >>
             1) ^
            (hash<glm::vec3>()(vertex.normal) << 1));
  }
};

template <> struct hash<vkr::VertexTextured3D> {
  size_t operator()(vkr::VertexTextured3D const &vertex) const {
    return (((hash<glm::vec3>()(vertex.pos) ^
              (hash<glm::vec3>()(vertex.color) << 1)) >>
             1) ^
            (hash<glm::vec2>()(vertex.texCoord) << 1));
  }
};

template <> struct hash<vkr::VertexNormalTexture3D> {
  size_t operator()(vkr::VertexNormalTexture3D const &vertex) const {
    return ((((hash<glm::vec3>()(vertex.pos) ^
               (hash<glm::vec3>()(vertex.color) << 1)) >>
              1) ^
             (hash<glm::vec3>()(vertex.normal) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};

} // namespace std
