#pragma once

#include "../ctx.hh"

namespace vkr {

class IndexBuffer {
public:
  IndexBuffer(const std::vector<uint16_t> &indices, VkDevice device,
              VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
              VkQueue graphicsQueue);
  IndexBuffer(const std::vector<uint16_t> &indices, const VulkanContext &ctx);
  ~IndexBuffer();

  IndexBuffer(const IndexBuffer &) = delete;
  IndexBuffer &operator=(const IndexBuffer &) = delete;

  IndexBuffer(IndexBuffer &&other) noexcept;
  IndexBuffer &operator=(IndexBuffer &&other) noexcept;

  void create();
  void destroy();
  void update(const std::vector<uint16_t> &indices);

  [[nodiscard]] std::vector<uint16_t> indices() const noexcept {
    return _indices;
  }
  [[nodiscard]] VkBuffer buffer() const noexcept { return _indexBuffer; }
  [[nodiscard]] VkDeviceMemory bufferMemory() const noexcept { return _memory; }

private:
  // components
  VkDevice device{VK_NULL_HANDLE};
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkCommandPool commandPool{VK_NULL_HANDLE};
  VkQueue graphicsQueue{VK_NULL_HANDLE};

  // dependencies
  std::vector<uint16_t> _indices{};
  VkBuffer _indexBuffer{VK_NULL_HANDLE};
  VkDeviceMemory _memory{VK_NULL_HANDLE};
};
} // namespace vkr
