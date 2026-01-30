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
  IndexBuffer &operator=(const IndexBuffer &) = delete;

  IndexBuffer(IndexBuffer &&other) noexcept;
  IndexBuffer &operator=(IndexBuffer &&other) noexcept;

  void create();
  void destroy();
  void update(const std::vector<uint16_t> &indices);

  [[nodiscard]] std::vector<uint16_t> indices() const noexcept {
    return indices_;
  }
  [[nodiscard]] VkBuffer buffer() const noexcept { return vk_index_buffer_; }
  [[nodiscard]] VkDeviceMemory bufferMemory() const noexcept {
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
