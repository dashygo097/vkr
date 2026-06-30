#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/gpu/buffer_utils.hh"
#include <glm/glm.hpp>

namespace vkr::resource {

class IVertexBuffer {
public:
  virtual ~IVertexBuffer() = default;

  [[nodiscard]] virtual auto buffer() const noexcept -> VkBuffer = 0;
  [[nodiscard]] virtual auto bufferMemory() const noexcept
      -> VkDeviceMemory = 0;
  [[nodiscard]] virtual auto vertexCount() const noexcept -> size_t = 0;

  virtual void updateRaw(const void *data, size_t count) = 0;
};

template <typename VertexType> class VertexBuffer : public IVertexBuffer {
public:
  explicit VertexBuffer(const core::Device &device,
                        const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool) {
    create();
  }

  ~VertexBuffer() override { destroy(); }

  VertexBuffer(const VertexBuffer &) = delete;
  auto operator=(const VertexBuffer &) -> VertexBuffer & = delete;

  void update(const std::vector<VertexType> &vertices) {
    if (vertices.empty()) {
      VKR_RES_ERROR("Cannot update vertex buffer with no vertices!");
    }

    VkDeviceSize newBufferSize = sizeof(VertexType) * vertices.size();
    VkDeviceSize oldBufferSize = sizeof(VertexType) * vertices_.size();

    // If size changed, recreate the buffer
    if (newBufferSize != oldBufferSize) {
      destroy();
      vertices_ = vertices;
      create();
      return;
    }

    // Size is the same, just update the data
    vertices_ = vertices;

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
    const auto *rawVertices = static_cast<const VertexType *>(data);
    std::vector<VertexType> vertices(rawVertices, rawVertices + count);
    update(vertices);
  }

  [[nodiscard]] auto buffer() const noexcept -> VkBuffer override {
    return vk_vertex_buffer_;
  }

  [[nodiscard]] auto bufferMemory() const noexcept -> VkDeviceMemory override {
    return vk_memory_;
  }

  [[nodiscard]] auto vertexCount() const noexcept -> size_t override {
    return vertices_.size();
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

} // namespace vkr::resource
