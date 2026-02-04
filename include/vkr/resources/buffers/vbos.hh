#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#define BIND_OBJECT(name)                                                      \
  bindingDescription.binding = 0;                                              \
  bindingDescription.stride = sizeof(name);                                    \
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

#define BIND_2D_ATTRS(name, obj, index)                                        \
  attributeDescriptions[index].binding = 0;                                    \
  attributeDescriptions[index].location = index;                               \
  attributeDescriptions[index].format = VK_FORMAT_R32G32_SFLOAT;               \
  attributeDescriptions[index].offset = offsetof(name, obj);

#define BIND_3D_ATTRS(name, obj, index)                                        \
  attributeDescriptions[index].binding = 0;                                    \
  attributeDescriptions[index].location = index;                               \
  attributeDescriptions[index].format = VK_FORMAT_R32G32B32_SFLOAT;            \
  attributeDescriptions[index].offset = offsetof(name, obj);

namespace vkr::resource {

struct Vertex2D {
public:
  glm::vec2 pos;
  glm::vec3 color;

  bool operator==(const Vertex2D &other) const {
    return pos == other.pos && color == other.color;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    BIND_OBJECT(Vertex2D)

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(2);

    BIND_2D_ATTRS(Vertex2D, pos, 0)
    BIND_3D_ATTRS(Vertex2D, color, 1)

    return attributeDescriptions;
  }
};

struct VertexTextured2D {
public:
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  bool operator==(const VertexTextured2D &other) const {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    BIND_OBJECT(VertexTextured2D)

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(3);

    BIND_2D_ATTRS(VertexTextured2D, pos, 0)
    BIND_3D_ATTRS(VertexTextured2D, color, 1)
    BIND_2D_ATTRS(VertexTextured2D, texCoord, 2)

    return attributeDescriptions;
  }
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

    BIND_OBJECT(Vertex3D)

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(2);

    BIND_3D_ATTRS(Vertex3D, pos, 0)
    BIND_3D_ATTRS(Vertex3D, color, 1)

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

    BIND_OBJECT(VertexNormal3D)

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(3);

    BIND_3D_ATTRS(VertexNormal3D, pos, 0)
    BIND_3D_ATTRS(VertexNormal3D, color, 1)
    BIND_3D_ATTRS(VertexNormal3D, normal, 2)

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

    BIND_OBJECT(VertexTextured3D)

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(3);

    BIND_3D_ATTRS(VertexTextured3D, pos, 0)
    BIND_3D_ATTRS(VertexTextured3D, color, 1)
    BIND_2D_ATTRS(VertexTextured3D, texCoord, 2)

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

    BIND_OBJECT(VertexNormalTexture3D)

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(4);

    BIND_3D_ATTRS(VertexNormalTexture3D, pos, 0)
    BIND_3D_ATTRS(VertexNormalTexture3D, color, 1)
    BIND_3D_ATTRS(VertexNormalTexture3D, normal, 2)
    BIND_2D_ATTRS(VertexNormalTexture3D, texCoord, 3)

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
