#pragma once

#include "../core/command_pool.hh"
#include "../core/device.hh"
#include "../logger.hh"
#include "./buffers/index_buffer.hh"
#include "./buffers/vertex_buffer.hh"

namespace vkr::resource {

template <typename VBOType> class Mesh {
public:
  explicit Mesh(const core::Device &device,
                const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool) {}
  ~Mesh() = default;

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

public:
  void load(const std::vector<VBOType> &vertices,
            const std::vector<uint16_t> &indices) {
    if (!vertex_buffer_ || !index_buffer_) {
      vertex_buffer_ = std::make_unique<VertexBufferBase<VBOType>>(
          device_, command_pool_, vertices);
      index_buffer_ = std::make_unique<IndexBuffer>(device_, command_pool_);
      index_buffer_->update(indices);
    } else {
      update(vertices, indices);
    }
  }
  void load(const std::string &meshFilePath);

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

  [[nodiscard]] std::shared_ptr<VertexBufferBase<VBOType>>
  vertexBuffer() const {
    return vertex_buffer_;
  }
  [[nodiscard]] std::shared_ptr<IndexBuffer> indexBuffer() const {
    return index_buffer_;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  std::shared_ptr<VertexBufferBase<VBOType>> vertex_buffer_{nullptr};
  std::shared_ptr<IndexBuffer> index_buffer_{nullptr};

  void checkDataLoaded() {
    if (!vertex_buffer_ || !index_buffer_) {
      VKR_RES_ERROR("Vertex or index buffer is not initialized!");
    }
  }
};
} // namespace vkr::resource
