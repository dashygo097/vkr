#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::scene {

struct VertexInputDesc {
  std::vector<VkVertexInputBindingDescription> bindings{};
  std::vector<VkVertexInputAttributeDescription> attributes{};

  [[nodiscard]] static auto none() -> VertexInputDesc { return {}; }

  [[nodiscard]] auto isEmpty() const noexcept -> bool {
    return bindings.empty() && attributes.empty();
  }

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineVertexInputStateCreateInfo {
    VkPipelineVertexInputStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    info.pVertexBindingDescriptions =
        bindings.empty() ? nullptr : bindings.data();
    info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributes.size());
    info.pVertexAttributeDescriptions =
        attributes.empty() ? nullptr : attributes.data();
    return info;
  }
};

struct Vertex2D {
  glm::vec2 pos{};
  glm::vec3 color{};

  Vertex2D() = default;
  explicit Vertex2D(glm::vec2 pos, glm::vec3 color = glm::vec3{0.0f})
      : pos(pos), color(color) {}

  [[nodiscard]] auto operator==(const Vertex2D &other) const -> bool {
    return pos == other.pos && color == other.color;
  }

  [[nodiscard]] static auto getBindingDescription(uint32_t binding = 0)
      -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = sizeof(Vertex2D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  [[nodiscard]] static auto getAttributeDescriptions(uint32_t binding = 0)
      -> std::vector<VkVertexInputAttributeDescription> {
    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = binding;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32_SFLOAT;
    posDesc.offset = offsetof(Vertex2D, pos);

    VkVertexInputAttributeDescription colorDesc{};
    colorDesc.binding = binding;
    colorDesc.location = 1;
    colorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorDesc.offset = offsetof(Vertex2D, color);

    return {posDesc, colorDesc};
  }

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> VertexInputDesc {
    VertexInputDesc desc{};
    desc.bindings.push_back(getBindingDescription(binding));
    desc.attributes = getAttributeDescriptions(binding);
    return desc;
  }
};

struct VertexTextured2D {
  glm::vec2 pos{};
  glm::vec3 color{};
  glm::vec2 texCoord{};

  VertexTextured2D() = default;
  explicit VertexTextured2D(glm::vec2 pos, glm::vec3 color = glm::vec3{0.0f},
                            glm::vec2 texCoord = glm::vec2{0.0f})
      : pos(pos), color(color), texCoord(texCoord) {}

  explicit VertexTextured2D(const Vertex2D &v)
      : pos(v.pos), color(v.color), texCoord(glm::vec2{0.0f}) {}

  [[nodiscard]] auto operator==(const VertexTextured2D &other) const -> bool {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord;
  }

  [[nodiscard]] static auto getBindingDescription(uint32_t binding = 0)
      -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = sizeof(VertexTextured2D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  [[nodiscard]] static auto getAttributeDescriptions(uint32_t binding = 0)
      -> std::vector<VkVertexInputAttributeDescription> {
    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = binding;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32_SFLOAT;
    posDesc.offset = offsetof(VertexTextured2D, pos);

    VkVertexInputAttributeDescription colorDesc{};
    colorDesc.binding = binding;
    colorDesc.location = 1;
    colorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorDesc.offset = offsetof(VertexTextured2D, color);

    VkVertexInputAttributeDescription texCoordDesc{};
    texCoordDesc.binding = binding;
    texCoordDesc.location = 2;
    texCoordDesc.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordDesc.offset = offsetof(VertexTextured2D, texCoord);

    return {posDesc, colorDesc, texCoordDesc};
  }

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> VertexInputDesc {
    VertexInputDesc desc{};
    desc.bindings.push_back(getBindingDescription(binding));
    desc.attributes = getAttributeDescriptions(binding);
    return desc;
  }
};

struct Vertex3D {
  glm::vec3 pos{};
  glm::vec3 color{};

  Vertex3D() = default;
  explicit Vertex3D(glm::vec3 pos, glm::vec3 color = glm::vec3{0.0f})
      : pos(pos), color(color) {}

  [[nodiscard]] auto operator==(const Vertex3D &other) const -> bool {
    return pos == other.pos && color == other.color;
  }

  [[nodiscard]] static auto getBindingDescription(uint32_t binding = 0)
      -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = sizeof(Vertex3D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  [[nodiscard]] static auto getAttributeDescriptions(uint32_t binding = 0)
      -> std::vector<VkVertexInputAttributeDescription> {
    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = binding;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    posDesc.offset = offsetof(Vertex3D, pos);

    VkVertexInputAttributeDescription colorDesc{};
    colorDesc.binding = binding;
    colorDesc.location = 1;
    colorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorDesc.offset = offsetof(Vertex3D, color);

    return {posDesc, colorDesc};
  }

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> VertexInputDesc {
    VertexInputDesc desc{};
    desc.bindings.push_back(getBindingDescription(binding));
    desc.attributes = getAttributeDescriptions(binding);
    return desc;
  }
};

struct VertexNormal3D {
  glm::vec3 pos{};
  glm::vec3 color{};
  glm::vec3 normal{};

  VertexNormal3D() = default;
  explicit VertexNormal3D(glm::vec3 pos, glm::vec3 color = glm::vec3{0.0f},
                          glm::vec3 normal = glm::vec3{0.0f})
      : pos(pos), color(color), normal(normal) {}

  explicit VertexNormal3D(const Vertex3D &v)
      : pos(v.pos), color(v.color), normal(glm::vec3{0.0f}) {}

  [[nodiscard]] auto operator==(const VertexNormal3D &other) const -> bool {
    return pos == other.pos && color == other.color && normal == other.normal;
  }

  [[nodiscard]] static auto getBindingDescription(uint32_t binding = 0)
      -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = sizeof(VertexNormal3D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  [[nodiscard]] static auto getAttributeDescriptions(uint32_t binding = 0)
      -> std::vector<VkVertexInputAttributeDescription> {
    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = binding;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    posDesc.offset = offsetof(VertexNormal3D, pos);

    VkVertexInputAttributeDescription colorDesc{};
    colorDesc.binding = binding;
    colorDesc.location = 1;
    colorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorDesc.offset = offsetof(VertexNormal3D, color);

    VkVertexInputAttributeDescription normalDesc{};
    normalDesc.binding = binding;
    normalDesc.location = 2;
    normalDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalDesc.offset = offsetof(VertexNormal3D, normal);

    return {posDesc, colorDesc, normalDesc};
  }

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> VertexInputDesc {
    VertexInputDesc desc{};
    desc.bindings.push_back(getBindingDescription(binding));
    desc.attributes = getAttributeDescriptions(binding);
    return desc;
  }
};

struct VertexTextured3D {
  glm::vec3 pos{};
  glm::vec3 color{};
  glm::vec2 texCoord{};

  VertexTextured3D() = default;
  explicit VertexTextured3D(glm::vec3 pos, glm::vec3 color = glm::vec3{0.0f},
                            glm::vec2 texCoord = glm::vec2{0.0f})
      : pos(pos), color(color), texCoord(texCoord) {}

  explicit VertexTextured3D(const Vertex3D &v)
      : pos(v.pos), color(v.color), texCoord(glm::vec2{0.0f}) {}

  [[nodiscard]] auto operator==(const VertexTextured3D &other) const -> bool {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord;
  }

  [[nodiscard]] static auto getBindingDescription(uint32_t binding = 0)
      -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = sizeof(VertexTextured3D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  [[nodiscard]] static auto getAttributeDescriptions(uint32_t binding = 0)
      -> std::vector<VkVertexInputAttributeDescription> {
    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = binding;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    posDesc.offset = offsetof(VertexTextured3D, pos);

    VkVertexInputAttributeDescription colorDesc{};
    colorDesc.binding = binding;
    colorDesc.location = 1;
    colorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorDesc.offset = offsetof(VertexTextured3D, color);

    VkVertexInputAttributeDescription texCoordDesc{};
    texCoordDesc.binding = binding;
    texCoordDesc.location = 2;
    texCoordDesc.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordDesc.offset = offsetof(VertexTextured3D, texCoord);

    return {posDesc, colorDesc, texCoordDesc};
  }

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> VertexInputDesc {
    VertexInputDesc desc{};
    desc.bindings.push_back(getBindingDescription(binding));
    desc.attributes = getAttributeDescriptions(binding);
    return desc;
  }
};

struct VertexNormalTexture3D {
  glm::vec3 pos{};
  glm::vec3 color{};
  glm::vec3 normal{};
  glm::vec2 texCoord{};

  VertexNormalTexture3D() = default;
  explicit VertexNormalTexture3D(glm::vec3 pos,
                                 glm::vec3 color = glm::vec3{0.0f},
                                 glm::vec3 normal = glm::vec3{0.0f},
                                 glm::vec2 texCoord = glm::vec2{0.0f})
      : pos(pos), color(color), normal(normal), texCoord(texCoord) {}

  explicit VertexNormalTexture3D(const Vertex3D &v)
      : pos(v.pos), color(v.color), normal(glm::vec3{0.0f}),
        texCoord(glm::vec2{0.0f}) {}

  explicit VertexNormalTexture3D(const VertexNormal3D &v)
      : pos(v.pos), color(v.color), normal(v.normal),
        texCoord(glm::vec2{0.0f}) {}

  explicit VertexNormalTexture3D(const VertexTextured3D &v)
      : pos(v.pos), color(v.color), normal(glm::vec3{0.0f}),
        texCoord(v.texCoord) {}

  [[nodiscard]] auto operator==(const VertexNormalTexture3D &other) const
      -> bool {
    return pos == other.pos && color == other.color && normal == other.normal &&
           texCoord == other.texCoord;
  }

  [[nodiscard]] static auto getBindingDescription(uint32_t binding = 0)
      -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = sizeof(VertexNormalTexture3D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  [[nodiscard]] static auto getAttributeDescriptions(uint32_t binding = 0)
      -> std::vector<VkVertexInputAttributeDescription> {
    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = binding;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    posDesc.offset = offsetof(VertexNormalTexture3D, pos);

    VkVertexInputAttributeDescription colorDesc{};
    colorDesc.binding = binding;
    colorDesc.location = 1;
    colorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorDesc.offset = offsetof(VertexNormalTexture3D, color);

    VkVertexInputAttributeDescription normalDesc{};
    normalDesc.binding = binding;
    normalDesc.location = 2;
    normalDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalDesc.offset = offsetof(VertexNormalTexture3D, normal);

    VkVertexInputAttributeDescription texCoordDesc{};
    texCoordDesc.binding = binding;
    texCoordDesc.location = 3;
    texCoordDesc.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordDesc.offset = offsetof(VertexNormalTexture3D, texCoord);

    return {posDesc, colorDesc, normalDesc, texCoordDesc};
  }

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> VertexInputDesc {
    VertexInputDesc desc{};
    desc.bindings.push_back(getBindingDescription(binding));
    desc.attributes = getAttributeDescriptions(binding);
    return desc;
  }
};

struct VertexSkybox3D {
  glm::vec3 pos{};

  VertexSkybox3D() = default;
  explicit VertexSkybox3D(glm::vec3 pos) : pos(pos) {}

  [[nodiscard]] auto operator==(const VertexSkybox3D &other) const -> bool {
    return pos == other.pos;
  }

  [[nodiscard]] static auto getBindingDescription(uint32_t binding = 0)
      -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = sizeof(VertexSkybox3D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  [[nodiscard]] static auto getAttributeDescriptions(uint32_t binding = 0)
      -> std::vector<VkVertexInputAttributeDescription> {
    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = binding;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    posDesc.offset = offsetof(VertexSkybox3D, pos);

    return {posDesc};
  }

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> VertexInputDesc {
    VertexInputDesc desc{};
    desc.bindings.push_back(getBindingDescription(binding));
    desc.attributes = getAttributeDescriptions(binding);
    return desc;
  }
};

[[nodiscard]] inline auto skyboxCubeVertices() -> std::vector<VertexSkybox3D> {
  return {
      VertexSkybox3D{{-1.0f, -1.0f, -1.0f}},
      VertexSkybox3D{{1.0f, -1.0f, -1.0f}},
      VertexSkybox3D{{1.0f, 1.0f, -1.0f}},
      VertexSkybox3D{{-1.0f, 1.0f, -1.0f}},
      VertexSkybox3D{{-1.0f, -1.0f, 1.0f}},
      VertexSkybox3D{{1.0f, -1.0f, 1.0f}},
      VertexSkybox3D{{1.0f, 1.0f, 1.0f}},
      VertexSkybox3D{{-1.0f, 1.0f, 1.0f}},
  };
}

[[nodiscard]] inline auto skyboxCubeIndices() -> std::vector<uint16_t> {
  return {
      0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 5, 4, 7, 7, 6, 5,
      4, 0, 3, 3, 7, 4, 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4,
  };
}

} // namespace vkr::scene

namespace std {

template <typename T, glm::qualifier Q> struct hash<glm::vec<2, T, Q>> {
  auto operator()(const glm::vec<2, T, Q> &vec) const noexcept -> size_t {
    size_t h1 = hash<T>{}(vec.x);
    size_t h2 = hash<T>{}(vec.y);
    return h1 ^ (h2 << 1);
  }
};

template <typename T, glm::qualifier Q> struct hash<glm::vec<3, T, Q>> {
  auto operator()(const glm::vec<3, T, Q> &vec) const noexcept -> size_t {
    size_t h1 = hash<T>{}(vec.x);
    size_t h2 = hash<T>{}(vec.y);
    size_t h3 = hash<T>{}(vec.z);
    return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
  }
};

template <> struct hash<vkr::scene::Vertex3D> {
  auto operator()(const vkr::scene::Vertex3D &vertex) const noexcept -> size_t {
    return ((hash<glm::vec3>{}(vertex.pos) ^
             (hash<glm::vec3>{}(vertex.color) << 1)) >>
            1);
  }
};

template <> struct hash<vkr::scene::VertexNormal3D> {
  auto operator()(const vkr::scene::VertexNormal3D &vertex) const noexcept
      -> size_t {
    return (((hash<glm::vec3>{}(vertex.pos) ^
              (hash<glm::vec3>{}(vertex.color) << 1)) >>
             1) ^
            (hash<glm::vec3>{}(vertex.normal) << 1));
  }
};

template <> struct hash<vkr::scene::VertexTextured3D> {
  auto operator()(const vkr::scene::VertexTextured3D &vertex) const noexcept
      -> size_t {
    return (((hash<glm::vec3>{}(vertex.pos) ^
              (hash<glm::vec3>{}(vertex.color) << 1)) >>
             1) ^
            (hash<glm::vec2>{}(vertex.texCoord) << 1));
  }
};

template <> struct hash<vkr::scene::VertexNormalTexture3D> {
  auto
  operator()(const vkr::scene::VertexNormalTexture3D &vertex) const noexcept
      -> size_t {
    return ((((hash<glm::vec3>{}(vertex.pos) ^
               (hash<glm::vec3>{}(vertex.color) << 1)) >>
              1) ^
             (hash<glm::vec3>{}(vertex.normal) << 1)) >>
            1) ^
           (hash<glm::vec2>{}(vertex.texCoord) << 1);
  }
};

template <> struct hash<vkr::scene::VertexSkybox3D> {
  auto operator()(const vkr::scene::VertexSkybox3D &vertex) const noexcept
      -> size_t {
    return hash<glm::vec3>{}(vertex.pos);
  }
};

} // namespace std
