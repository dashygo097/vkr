#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/scene/geometry/index_buffer.hh"
#include "vkr/scene/geometry/vertex_buffer.hh"
#include "vkr/scene/transform.hh"
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <vector>

namespace vkr::scene {

class IMesh {
public:
  virtual ~IMesh() = default;

  [[nodiscard]] auto transform() noexcept -> Transform & { return transform_; }
  [[nodiscard]] auto transform() const noexcept -> const Transform & {
    return transform_;
  }

  [[nodiscard]] virtual auto vertexBufferBase() const
      -> std::optional<std::reference_wrapper<const IVertexBuffer>> = 0;
  [[nodiscard]] virtual auto indexBuffer() const
      -> std::optional<std::reference_wrapper<const IndexBuffer>> = 0;

  [[nodiscard]] auto isValid() const -> bool {
    return vertexBufferBase().has_value() && indexBuffer().has_value();
  }

private:
  Transform transform_{};
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
          std::make_unique<VertexBuffer<VBOType>>(device_, command_pool_);
      index_buffer_ = std::make_unique<IndexBuffer>(device_, command_pool_);
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
    std::unordered_map<VBOType, uint16_t> uniqueVertices;

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

        const VBOType vertex{
            VertexNormalTexture3D{pos, color, normal, texCoord}};
        const auto existing = uniqueVertices.find(vertex);
        if (existing != uniqueVertices.end()) {
          indices.push_back(existing->second);
          continue;
        }

        if (vertices.size() > std::numeric_limits<uint16_t>::max()) {
          VKR_RES_ERROR("Mesh exceeds the 16-bit index capacity of {} vertices",
                        static_cast<size_t>(
                            std::numeric_limits<uint16_t>::max()) + 1);
        }

        const auto vertexIndex = static_cast<uint16_t>(vertices.size());
        uniqueVertices.emplace(vertex, vertexIndex);
        vertices.push_back(vertex);
        indices.push_back(vertexIndex);
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
      -> std::optional<std::reference_wrapper<const VertexBuffer<VBOType>>> {
    if (!vertex_buffer_) {
      return std::nullopt;
    }

    return *vertex_buffer_;
  }

  [[nodiscard]] auto vertexBufferBase() const
      -> std::optional<std::reference_wrapper<const IVertexBuffer>> override {
    if (!vertex_buffer_) {
      return std::nullopt;
    }

    return *vertex_buffer_;
  }

  [[nodiscard]] auto indexBuffer() const
      -> std::optional<std::reference_wrapper<const IndexBuffer>> override {
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
  std::unique_ptr<VertexBuffer<VBOType>> vertex_buffer_;
  std::unique_ptr<IndexBuffer> index_buffer_;

  void checkDataLoaded() {
    if (!vertex_buffer_ || !index_buffer_) {
      VKR_RES_ERROR("Vertex or index buffer is not initialized!");
    }
  }
};
} // namespace vkr::scene
