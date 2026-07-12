#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/vbos.hh"
#include "vkr/resource/gpu/buffer_utils.hh"
#include <cstring>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::resource {

class IVertexBuffer {
public:
  virtual ~IVertexBuffer() = default;

  [[nodiscard]] virtual auto buffer() const noexcept -> const VkBuffer & = 0;
  [[nodiscard]] virtual auto memory() const noexcept
      -> const VkDeviceMemory & = 0;

  [[nodiscard]] virtual auto vertexCount() const noexcept -> size_t = 0;

  [[nodiscard]] virtual auto vertexInputDesc() const -> VertexInputDesc = 0;

  virtual void updateRaw(const void *data, size_t count) = 0;
};

template <typename VertexType> class VertexBuffer : public IVertexBuffer {
public:
  explicit VertexBuffer(const core::Device &device,
                        const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool) {}

  ~VertexBuffer() override { destroy(); }

  VertexBuffer(const VertexBuffer &) = delete;
  auto operator=(const VertexBuffer &) -> VertexBuffer & = delete;

  void update(const std::vector<VertexType> &vertices) {
    if (vertices.empty()) {
      VKR_RES_ERROR("Cannot update vertex buffer with no vertices!");
    }

    const auto newBufferSize =
        static_cast<VkDeviceSize>(sizeof(VertexType) * vertices.size());
    const auto oldBufferSize =
        static_cast<VkDeviceSize>(sizeof(VertexType) * vertices_.size());

    vertices_ = vertices;

    if (newBufferSize != oldBufferSize || vk_vertex_buffer_ == VK_NULL_HANDLE) {
      destroy();
      create();
      return;
    }

    upload(vertices_.data(), newBufferSize);
  }

  void updateRaw(const void *data, size_t count) override {
    if (data == nullptr || count == 0) {
      VKR_RES_ERROR("Cannot update vertex buffer with invalid raw data!");
    }

    const auto *rawVertices = static_cast<const VertexType *>(data);
    std::vector<VertexType> vertices(rawVertices, rawVertices + count);
    update(vertices);
  }

  [[nodiscard]] auto buffer() const noexcept -> const VkBuffer & override {
    return vk_vertex_buffer_;
  }

  [[nodiscard]] auto memory() const noexcept
      -> const VkDeviceMemory & override {
    return vk_memory_;
  }

  [[nodiscard]] auto vertexCount() const noexcept -> size_t override {
    return vertices_.size();
  }

  [[nodiscard]] auto vertexInputDesc() const -> VertexInputDesc override {
    return VertexType::vertexInputDesc();
  }

  [[nodiscard]] auto vertices() const noexcept
      -> const std::vector<VertexType> & {
    return vertices_;
  }

protected:
  void create() {
    if (vertices_.empty()) {
      return;
    }

    const auto bufferSize =
        static_cast<VkDeviceSize>(sizeof(VertexType) * vertices_.size());

    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_vertex_buffer_,
                 vk_memory_, device_.device(), device_.physicalDevice());

    upload(vertices_.data(), bufferSize);
  }

  void destroy() {
    if (vk_vertex_buffer_ != VK_NULL_HANDLE) {
      vkDestroyBuffer(device_.device(), vk_vertex_buffer_, nullptr);
      vk_vertex_buffer_ = VK_NULL_HANDLE;
    }

    if (vk_memory_ != VK_NULL_HANDLE) {
      vkFreeMemory(device_.device(), vk_memory_, nullptr);
      vk_memory_ = VK_NULL_HANDLE;
    }
  }

  void upload(const VertexType *vertices, VkDeviceSize bufferSize) {
    if (vertices == nullptr || bufferSize == 0) {
      VKR_RES_ERROR("Cannot upload empty vertex buffer data!");
    }

    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory, device_.device(),
                 device_.physicalDevice());

    void *mapped{nullptr};
    vkMapMemory(device_.device(), stagingBufferMemory, 0, bufferSize, 0,
                &mapped);
    std::memcpy(mapped, vertices, static_cast<size_t>(bufferSize));
    vkUnmapMemory(device_.device(), stagingBufferMemory);

    copyBuffer(stagingBuffer, vk_vertex_buffer_, bufferSize,
               command_pool_.commandPool(), device_.graphicsQueue(),
               device_.device());

    vkDestroyBuffer(device_.device(), stagingBuffer, nullptr);
    vkFreeMemory(device_.device(), stagingBufferMemory, nullptr);
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

} // namespace vkr::resource
