#pragma once

#include "../../core/command_pool.hh"
#include "../../core/device.hh"
#include "./buffer_utils.hh"
#include <glm/glm.hpp>

namespace vkr::resource {

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
  explicit VertexBufferBase(const core::Device &device,
                            const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool) {
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
    VkDeviceSize oldBufferSize = sizeof(VertexType) * vertices_.size();

    // If size changed, recreate the buffer
    if (newBufferSize != oldBufferSize) {
      destroy();
      vertices_ = newVertices;
      create();
      return;
    }

    // Size is the same, just update the data
    vertices_ = newVertices;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(newBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory, device_.device(),
                 device_.physicalDevice());

    // Copy new vertex data to staging buffer
    void *data;
    vkMapMemory(device_.device(), stagingBufferMemory, 0, newBufferSize, 0,
                &data);
    memcpy(data, vertices_.data(), static_cast<size_t>(newBufferSize));
    vkUnmapMemory(device_.device(), stagingBufferMemory);

    // Copy from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, vk_vertex_buffer_, newBufferSize,
               command_pool_.commandPool(), device_.graphicsQueue(),
               device_.device());

    // Clean up staging buffer
    vkDestroyBuffer(device_.device(), stagingBuffer, nullptr);
    vkFreeMemory(device_.device(), stagingBufferMemory, nullptr);
  }

  void updateRaw(const void *data, size_t count) override {
    const VertexType *vertices = static_cast<const VertexType *>(data);
    std::vector<VertexType> newVertices(vertices, vertices + count);
    update(newVertices);
  }

  [[nodiscard]] VkBuffer buffer() const noexcept override {
    return vk_vertex_buffer_;
  }

  [[nodiscard]] VkDeviceMemory bufferMemory() const noexcept override {
    return vk_memory_;
  }

  [[nodiscard]] size_t vertexCount() const noexcept override {
    return vertices_.size();
  }

  [[nodiscard]] const std::vector<VertexType> &vertices() const noexcept {
    return vertices_;
  }

protected:
  void create() {
    if (vertices_.empty()) {
      return;
    }

    VkDeviceSize bufferSize = sizeof(VertexType) * vertices_.size();

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory, device_.device(),
                 device_.physicalDevice());

    // Copy vertex data to staging buffer
    void *data;
    vkMapMemory(device_.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices_.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device_.device(), stagingBufferMemory);

    // Create vertex buffer in device local memory
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_vertex_buffer_,
                 vk_memory_, device_.device(), device_.physicalDevice());

    // Copy from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, vk_vertex_buffer_, bufferSize,
               command_pool_.commandPool(), device_.graphicsQueue(),
               device_.device());

    // Clean up staging buffer
    vkDestroyBuffer(device_.device(), stagingBuffer, nullptr);
    vkFreeMemory(device_.device(), stagingBufferMemory, nullptr);
  }

  void destroy() {
    if (vk_memory_ != VK_NULL_HANDLE) {
      vkFreeMemory(device_.device(), vk_memory_, nullptr);
      vk_memory_ = VK_NULL_HANDLE;
    }
    if (vk_vertex_buffer_ != VK_NULL_HANDLE) {
      vkDestroyBuffer(device_.device(), vk_vertex_buffer_, nullptr);
      vk_vertex_buffer_ = VK_NULL_HANDLE;
    }
  }

protected:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  std::vector<VertexType> vertices_{};
  VkBuffer vk_vertex_buffer_{VK_NULL_HANDLE};
  VkDeviceMemory vk_memory_{VK_NULL_HANDLE};
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

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(2);

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

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(3);

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

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(3);

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

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(4);

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

} // namespace vkr::resource

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

template <> struct hash<vkr::resource::Vertex3D> {
  size_t operator()(vkr::resource::Vertex3D const &vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1);
  }
};

template <> struct hash<vkr::resource::VertexNormal3D> {
  size_t operator()(vkr::resource::VertexNormal3D const &vertex) const {
    return (((hash<glm::vec3>()(vertex.pos) ^
              (hash<glm::vec3>()(vertex.color) << 1)) >>
             1) ^
            (hash<glm::vec3>()(vertex.normal) << 1));
  }
};

template <> struct hash<vkr::resource::VertexTextured3D> {
  size_t operator()(vkr::resource::VertexTextured3D const &vertex) const {
    return (((hash<glm::vec3>()(vertex.pos) ^
              (hash<glm::vec3>()(vertex.color) << 1)) >>
             1) ^
            (hash<glm::vec2>()(vertex.texCoord) << 1));
  }
};

template <> struct hash<vkr::resource::VertexNormalTexture3D> {
  size_t operator()(vkr::resource::VertexNormalTexture3D const &vertex) const {
    return ((((hash<glm::vec3>()(vertex.pos) ^
               (hash<glm::vec3>()(vertex.color) << 1)) >>
              1) ^
             (hash<glm::vec3>()(vertex.normal) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};

} // namespace std
