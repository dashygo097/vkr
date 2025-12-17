#pragma once

#include "../../core/command_pool.hh"
#include "../../core/device.hh"
#include "../buffers/index_buffer.hh"
#include "../buffers/vertex_buffer.hh"

namespace vkr::geometry {
class Mesh {
public:
  explicit Mesh(const Device &device, const CommandPool &commandPool);
  ~Mesh() = default;

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
  const Device &device;
  const CommandPool &commandPool;

  // components
  std::shared_ptr<VertexBuffer> _vertexBuffer;
  std::shared_ptr<IndexBuffer> _indexBuffer;

  void checkDataLoaded() {
    if (!_vertexBuffer || !_indexBuffer) {
      throw std::runtime_error("Vertex or index buffer is not initialized");
    }
  }
};
} // namespace vkr::geometry
