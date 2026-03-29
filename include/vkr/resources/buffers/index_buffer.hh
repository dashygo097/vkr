#pragma once

#include "../../core/command_pool.hh"
#include "../../core/device.hh"

namespace vkr::resource {

class IndexBuffer {
public:
  explicit IndexBuffer(const core::Device &device,
                       const core::CommandPool &commandPool);
  ~IndexBuffer();

  IndexBuffer(const IndexBuffer &) = delete;
  auto operator=(const IndexBuffer &) -> IndexBuffer & = delete;

  IndexBuffer(IndexBuffer &&other) noexcept;
  auto operator=(IndexBuffer &&other) noexcept -> IndexBuffer &;

  void create();
  void destroy();
  void update(const std::vector<uint16_t> &indices);

  [[nodiscard]] auto indices() const noexcept -> std::vector<uint16_t> {
    return indices_;
  }
  [[nodiscard]] auto buffer() const noexcept -> VkBuffer {
    return vk_index_buffer_;
  }
  [[nodiscard]] auto bufferMemory() const noexcept -> VkDeviceMemory {
    return vk_memory_;
  }

private:
  // components
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // dependencies
  std::vector<uint16_t> indices_{};
  VkBuffer vk_index_buffer_{VK_NULL_HANDLE};
  VkDeviceMemory vk_memory_{VK_NULL_HANDLE};
};
} // namespace vkr::resource
