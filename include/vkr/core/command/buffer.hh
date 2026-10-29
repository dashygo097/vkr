#pragma once

#include "vkr/core/device.hh"

namespace vkr::core {

class CommandPool;

class CommandBuffer {
public:
  explicit CommandBuffer(const Device &device,
                         const CommandPool &commandPool) noexcept;
  ~CommandBuffer();

  CommandBuffer(const CommandBuffer &) = delete;
  auto operator=(const CommandBuffer &) -> CommandBuffer & = delete;

  CommandBuffer(CommandBuffer &&other) noexcept;
  auto operator=(CommandBuffer &&) -> CommandBuffer & = delete;

  void begin(VkCommandBufferUsageFlags flags = 0);
  void end();
  void reset(VkCommandBufferResetFlags flags = 0);
  void destroy() noexcept;

  [[nodiscard]] auto buffer() const noexcept -> VkCommandBuffer {
    return vk_command_buffer_;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return vk_command_buffer_ != VK_NULL_HANDLE;
  }

private:
  friend class CommandPool;

  // dependencies
  const Device &device_;
  const CommandPool &command_pool_;

  // components
  VkCommandBuffer vk_command_buffer_{VK_NULL_HANDLE};
};

} // namespace vkr::core
