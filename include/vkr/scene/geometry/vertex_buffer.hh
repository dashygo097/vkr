#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/buffer.hh"
#include "vkr/scene/geometry/vbos.hh"
#include <cstring>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::scene {

class IVertexBuffer {
public:
  virtual ~IVertexBuffer() = default;

  [[nodiscard]] virtual auto buffer() const -> const VkBuffer & = 0;
  [[nodiscard]] virtual auto memory() const -> const VkDeviceMemory & = 0;

  [[nodiscard]] virtual auto vertexCount() const noexcept -> size_t = 0;

  [[nodiscard]] virtual auto vertexInputDesc() const -> VertexInputDesc = 0;

  virtual void updateRaw(const void *data, size_t count) = 0;
};

template <typename VertexType> class VertexBuffer : public IVertexBuffer {
public:
  explicit VertexBuffer(const core::Device &device,
                        const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool),
        target_(std::make_unique<resource::Buffer>(device)) {}

  ~VertexBuffer() override = default;

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

    if (newBufferSize != oldBufferSize || !target_->isValid()) {
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

  [[nodiscard]] auto buffer() const -> const VkBuffer & override {
    return target_->buffer();
  }

  [[nodiscard]] auto memory() const -> const VkDeviceMemory & override {
    return target_->memory();
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

    target_->update(bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    upload(vertices_.data(), bufferSize);
  }

  void destroy() { target_->destroy(); }

  void upload(const VertexType *vertices, VkDeviceSize bufferSize) {
    if (vertices == nullptr || bufferSize == 0) {
      VKR_RES_ERROR("Cannot upload empty vertex buffer data!");
    }

    resource::Buffer staging{device_, bufferSize,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    staging.write(vertices, bufferSize);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool_.commandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    if (vkAllocateCommandBuffers(device_.device(), &allocInfo,
                                 &commandBuffer) != VK_SUCCESS) {
      VKR_RES_ERROR("Failed to allocate vertex upload command buffer");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                           &commandBuffer);
      VKR_RES_ERROR("Failed to begin vertex upload command buffer");
    }

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, staging.buffer(), target_->buffer(), 1,
                    &copyRegion);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                           &commandBuffer);
      VKR_RES_ERROR("Failed to end vertex upload command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(command_pool_.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
        VK_SUCCESS) {
      vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                           &commandBuffer);
      VKR_RES_ERROR("Failed to submit vertex upload command buffer");
    }

    vkQueueWaitIdle(command_pool_.queue());
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
  }

protected:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  std::vector<VertexType> vertices_{};
  std::unique_ptr<resource::Buffer> target_{};
};

} // namespace vkr::scene
