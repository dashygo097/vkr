#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/scene/geometry/index_buffer.hh"
#include "vkr/scene/geometry/vertex_buffer.hh"
#include "vkr/scene/geometry/vertex_import.hh"
#include "vkr/scene/transform.hh"
#include "vkr/util/obj_loader.hh"
#include <functional>
#include <memory>
#include <optional>
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

  auto stream(uint32_t binding, const IVertexBuffer &buffer) -> IMesh & {
    streams_.insert_or_assign(binding, std::cref(buffer));
    return *this;
  }
  auto stream(uint32_t, const IVertexBuffer &&) -> IMesh & = delete;

  [[nodiscard]] auto stream(uint32_t binding) const
      -> std::optional<std::reference_wrapper<const IVertexBuffer>> {
    const auto found = streams_.find(binding);
    if (found != streams_.end()) {
      return found->second;
    }
    return binding == 0 ? vertexBufferBase() : std::nullopt;
  }

  [[nodiscard]] auto isValid() const -> bool {
    return vertexBufferBase().has_value() && indexBuffer().has_value();
  }

private:
  Transform transform_{};
  std::unordered_map<uint32_t, std::reference_wrapper<const IVertexBuffer>>
      streams_{};
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
  void load(const std::filesystem::path &path, const VertexImportDesc &desc) {
    const auto source = util::loadObj(path);
    const auto vertices = desc.pack<VBOType>(source.vertices);
    load(vertices, source.indices);
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
