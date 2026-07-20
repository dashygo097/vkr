#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/index_buffer.hh"
#include "vkr/resource/buffer/vertex_buffer.hh"
#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <tiny_obj_loader.h>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace vkr::scene {

namespace {

template <typename...> struct AlwaysFalse : std::false_type {};

template <typename VertexType, typename = void>
struct HasPosition : std::false_type {};

template <typename VertexType>
struct HasPosition<VertexType,
                   std::void_t<decltype(std::declval<VertexType &>().pos)>>
    : std::true_type {};

template <typename VertexType, typename = void>
struct HasColor : std::false_type {};

template <typename VertexType>
struct HasColor<VertexType,
                std::void_t<decltype(std::declval<VertexType &>().color)>>
    : std::true_type {};

template <typename VertexType, typename = void>
struct HasNormal : std::false_type {};

template <typename VertexType>
struct HasNormal<VertexType,
                 std::void_t<decltype(std::declval<VertexType &>().normal)>>
    : std::true_type {};

template <typename VertexType, typename = void>
struct HasTexCoord : std::false_type {};

template <typename VertexType>
struct HasTexCoord<VertexType,
                   std::void_t<decltype(std::declval<VertexType &>().texCoord)>>
    : std::true_type {};

template <typename VertexType>
constexpr bool UseHashDedup =
    std::is_default_constructible_v<std::hash<VertexType>> &&
    std::is_invocable_r_v<size_t, std::hash<VertexType>, const VertexType &>;

template <typename VertexType>
void assignPosition(VertexType &vertex, glm::vec3 pos) {
  static_assert(HasPosition<VertexType>::value,
                "Mesh OBJ loading requires vertex types to expose a pos field");

  using PositionType = std::decay_t<decltype(vertex.pos)>;

  if constexpr (std::is_same_v<PositionType, glm::vec3>) {
    vertex.pos = pos;
  } else if constexpr (std::is_same_v<PositionType, glm::vec2>) {
    vertex.pos = {pos.x, pos.y};
  } else {
    static_assert(
        AlwaysFalse<PositionType>::value,
        "Mesh OBJ loading only supports glm::vec2/glm::vec3 pos fields");
  }
}

template <typename VertexType>
void assignColor(VertexType &vertex, glm::vec3 color) {
  if constexpr (HasColor<VertexType>::value) {
    using ColorType = std::decay_t<decltype(vertex.color)>;

    if constexpr (std::is_same_v<ColorType, glm::vec3>) {
      vertex.color = color;
    } else if constexpr (std::is_same_v<ColorType, glm::vec4>) {
      vertex.color = {color, 1.0f};
    } else {
      static_assert(
          AlwaysFalse<ColorType>::value,
          "Mesh OBJ loading only supports glm::vec3/glm::vec4 color fields");
    }
  }
}

template <typename VertexType>
void assignNormal(VertexType &vertex, glm::vec3 normal) {
  if constexpr (HasNormal<VertexType>::value) {
    using NormalType = std::decay_t<decltype(vertex.normal)>;

    if constexpr (std::is_same_v<NormalType, glm::vec3>) {
      vertex.normal = normal;
    } else {
      static_assert(AlwaysFalse<NormalType>::value,
                    "Mesh OBJ loading only supports glm::vec3 normal fields");
    }
  }
}

template <typename VertexType>
void assignTexCoord(VertexType &vertex, glm::vec2 texCoord) {
  if constexpr (HasTexCoord<VertexType>::value) {
    using TexCoordType = std::decay_t<decltype(vertex.texCoord)>;

    if constexpr (std::is_same_v<TexCoordType, glm::vec2>) {
      vertex.texCoord = texCoord;
    } else if constexpr (std::is_same_v<TexCoordType, glm::vec3>) {
      vertex.texCoord = {texCoord.x, texCoord.y, 0.0f};
    } else {
      static_assert(
          AlwaysFalse<TexCoordType>::value,
          "Mesh OBJ loading only supports glm::vec2/glm::vec3 texCoord fields");
    }
  }
}

template <typename VertexType, bool UseHash = UseHashDedup<VertexType>>
class VertexDeduplicator {
public:
  auto indexFor(const VertexType &vertex, std::vector<VertexType> &vertices)
      -> uint16_t {
    auto it = std::find(vertices.begin(), vertices.end(), vertex);
    if (it != vertices.end()) {
      return static_cast<uint16_t>(std::distance(vertices.begin(), it));
    }

    if (vertices.size() > std::numeric_limits<uint16_t>::max()) {
      VKR_RES_ERROR("Mesh has more than {} unique vertices",
                    std::numeric_limits<uint16_t>::max());
    }

    vertices.push_back(vertex);
    return static_cast<uint16_t>(vertices.size() - 1);
  }
};

template <typename VertexType> class VertexDeduplicator<VertexType, true> {
public:
  auto indexFor(const VertexType &vertex, std::vector<VertexType> &vertices)
      -> uint16_t {
    auto it = unique_vertices_.find(vertex);
    if (it != unique_vertices_.end()) {
      return it->second;
    }

    if (vertices.size() > std::numeric_limits<uint16_t>::max()) {
      VKR_RES_ERROR("Mesh has more than {} unique vertices",
                    std::numeric_limits<uint16_t>::max());
    }

    const auto index = static_cast<uint16_t>(vertices.size());
    vertices.push_back(vertex);
    unique_vertices_.emplace(vertex, index);
    return index;
  }

private:
  std::unordered_map<VertexType, uint16_t> unique_vertices_{};
};

} // namespace

class IMesh {
public:
  virtual ~IMesh() = default;

  [[nodiscard]] virtual auto vertexBufferBase() const
      -> std::optional<std::reference_wrapper<const resource::IVertexBuffer>> =
      0;
  [[nodiscard]] virtual auto indexBuffer() const
      -> std::optional<std::reference_wrapper<const resource::IndexBuffer>> = 0;

  [[nodiscard]] auto isValid() const -> bool {
    return vertexBufferBase().has_value() && indexBuffer().has_value();
  }
};

template <typename VBOType> class Mesh final : public IMesh {
public:
  explicit Mesh(const core::Device &device,
                const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool) {}
  ~Mesh() override = default;

  Mesh(const Mesh &) = delete;
  auto operator=(const Mesh &) -> Mesh & = delete;

public:
  void load(const std::vector<VBOType> &vertices,
            const std::vector<uint16_t> &indices) {
    if (!vertex_buffer_ || !index_buffer_) {
      vertex_buffer_ =
          std::make_unique<resource::VertexBuffer<VBOType>>(device_,
                                                            command_pool_);
      index_buffer_ =
          std::make_unique<resource::IndexBuffer>(device_, command_pool_);
      vertex_buffer_->update(vertices);
      index_buffer_->update(indices);
    } else {
      update(vertices, indices);
    }
  }
  void load(const std::string &meshFilePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    const std::string::size_type separator = meshFilePath.find_last_of("/\\");
    const std::string dir = separator == std::string::npos
                                ? std::string{}
                                : meshFilePath.substr(0, separator);
    const char *basepath = dir.empty() ? nullptr : dir.c_str();

    const bool success =
        tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                         meshFilePath.c_str(), basepath, true);

    if (!warn.empty()) {
      VKR_RES_WARN("OBJ warning: {}", warn);
    }

    if (!err.empty()) {
      VKR_RES_ERROR("OBJ error: {}", err);
    }

    if (!success) {
      VKR_RES_ERROR("Failed to load OBJ file: {}", meshFilePath);
    }

    std::vector<VBOType> vertices;
    std::vector<uint16_t> indices;
    VertexDeduplicator<VBOType> uniqueVertices;

    for (const auto &shape : shapes) {
      for (const auto &index : shape.mesh.indices) {
        glm::vec3 pos{};
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        glm::vec3 normal{};
        glm::vec2 texCoord{};

        if (index.vertex_index >= 0) {
          const auto vertexOffset = static_cast<size_t>(3 * index.vertex_index);
          if (attrib.vertices.size() > vertexOffset + 2) {
            pos = {attrib.vertices[vertexOffset + 0],
                   attrib.vertices[vertexOffset + 1],
                   attrib.vertices[vertexOffset + 2]};
          }

          if (attrib.colors.size() > vertexOffset + 2) {
            color = {attrib.colors[vertexOffset + 0],
                     attrib.colors[vertexOffset + 1],
                     attrib.colors[vertexOffset + 2]};
          }
        }

        if (index.normal_index >= 0) {
          const auto normalOffset = static_cast<size_t>(3 * index.normal_index);
          if (attrib.normals.size() > normalOffset + 2) {
            normal = {attrib.normals[normalOffset + 0],
                      attrib.normals[normalOffset + 1],
                      attrib.normals[normalOffset + 2]};
          }
        }

        if (index.texcoord_index >= 0) {
          const auto texCoordOffset =
              static_cast<size_t>(2 * index.texcoord_index);
          if (attrib.texcoords.size() > texCoordOffset + 1) {
            texCoord = {attrib.texcoords[texCoordOffset + 0],
                        attrib.texcoords[texCoordOffset + 1]};
          }
        }

        VBOType vertex{};
        assignPosition(vertex, pos);
        assignColor(vertex, color);
        assignNormal(vertex, normal);
        assignTexCoord(vertex, texCoord);

        indices.push_back(uniqueVertices.indexFor(vertex, vertices));
      }
    }

    load(vertices, indices);

    VKR_RES_INFO("Loaded mesh: {} vertices, {} indices", vertices.size(),
                 indices.size());
  }

  void update(const std::vector<VBOType> &vertices,
              const std::vector<uint16_t> &indices) {
    checkDataLoaded();
    vertex_buffer_->update(vertices);
    index_buffer_->update(indices);
  }
  void update(const std::vector<VBOType> &vertices) {
    checkDataLoaded();
    vertex_buffer_->update(vertices);
  }
  void update(const std::vector<uint16_t> &indices) {
    checkDataLoaded();
    index_buffer_->update(indices);
  }

  [[nodiscard]] auto vertexBuffer() const
      -> std::optional<
          std::reference_wrapper<const resource::VertexBuffer<VBOType>>> {
    if (!vertex_buffer_) {
      return std::nullopt;
    }

    return *vertex_buffer_;
  }

  [[nodiscard]] auto vertexBufferBase() const
      -> std::optional<std::reference_wrapper<const resource::IVertexBuffer>>
          override {
    if (!vertex_buffer_) {
      return std::nullopt;
    }

    return *vertex_buffer_;
  }

  [[nodiscard]] auto indexBuffer() const
      -> std::optional<std::reference_wrapper<const resource::IndexBuffer>>
          override {
    if (!index_buffer_) {
      return std::nullopt;
    }

    return *index_buffer_;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  std::unique_ptr<resource::VertexBuffer<VBOType>> vertex_buffer_;
  std::unique_ptr<resource::IndexBuffer> index_buffer_;

  void checkDataLoaded() {
    if (!vertex_buffer_ || !index_buffer_) {
      VKR_RES_ERROR("Vertex or index buffer is not initialized!");
    }
  }
};
} // namespace vkr::scene
