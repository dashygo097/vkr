#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/buffers/buffer.hh"
#include <vector>

namespace vkr::resource {

class IndexBuffer {
public:
  explicit IndexBuffer(const core::Device &device,
                       const core::CommandPool &commandPool);
  ~IndexBuffer();

  IndexBuffer(const IndexBuffer &) = delete;
  auto operator=(const IndexBuffer &) -> IndexBuffer & = delete;

  IndexBuffer(IndexBuffer &&other) noexcept = delete;
  auto operator=(IndexBuffer &&other) noexcept -> IndexBuffer & = delete;

  void create();
  void destroy();
  void update(const std::vector<uint16_t> &indices);

  [[nodiscard]] auto indices() const noexcept -> std::vector<uint16_t> {
    return indices_;
  }
  [[nodiscard]] auto buffer() const noexcept -> VkBuffer {
    return target_.buffer();
  }
  [[nodiscard]] auto bufferMemory() const noexcept -> VkDeviceMemory {
    return target_.memory();
  }

private:
  // components
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // dependencies
  std::vector<uint16_t> indices_{};
  Buffer target_;
};
} // namespace vkr::resource
