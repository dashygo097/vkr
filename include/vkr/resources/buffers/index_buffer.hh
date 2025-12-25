#pragma once

#include "../../core/command_pool.hh"
#include "../../core/device.hh"

namespace vkr {

class IndexBuffer {
public:
  explicit IndexBuffer(const Device &device, const CommandPool &commandPool);
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
  const Device &device;
  const CommandPool &commandPool;

  // dependencies
  std::vector<uint16_t> _indices{};
  VkBuffer _indexBuffer{VK_NULL_HANDLE};
  VkDeviceMemory _memory{VK_NULL_HANDLE};
};
} // namespace vkr
