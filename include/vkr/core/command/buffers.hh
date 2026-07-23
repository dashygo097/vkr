#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include <cstdint>
#include <vector>

namespace vkr::core {
struct CommandBuffersDesc {
  uint32_t size{0};

  [[nodiscard]] auto isValid() const noexcept -> bool { return size > 0; }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("size", size);
  }
};

class CommandBuffers {
public:
  explicit CommandBuffers(const Device &device, const CommandPool &commandPool);
  ~CommandBuffers();

  CommandBuffers(const CommandBuffers &) = delete;
  auto operator=(const CommandBuffers &) -> CommandBuffers & = delete;

  void update(const CommandBuffersDesc &desc);
  void create();
  void destroy();

  [[nodiscard]] auto desc() const noexcept -> const CommandBuffersDesc & {
    return desc_;
  }

  [[nodiscard]] auto buffers() noexcept -> std::vector<VkCommandBuffer> & {
    return vk_command_buffers_;
  }

  [[nodiscard]] auto buffers() const noexcept
      -> const std::vector<VkCommandBuffer> & {
    return vk_command_buffers_;
  }

  [[nodiscard]] auto buffer(uint32_t index) -> VkCommandBuffer & {
    return vk_command_buffers_.at(index);
  }

  [[nodiscard]] auto buffer(uint32_t index) const -> VkCommandBuffer {
    return vk_command_buffers_.at(index);
  }

  [[nodiscard]] auto size() const noexcept -> uint32_t {
    return static_cast<uint32_t>(vk_command_buffers_.size());
  }

  [[nodiscard]] auto empty() const noexcept -> bool {
    return vk_command_buffers_.empty();
  }

private:
  // dependencies
  const Device &device_;
  const CommandPool &command_pool_;

  // components
  CommandBuffersDesc desc_{};
  std::vector<VkCommandBuffer> vk_command_buffers_{};
};

} // namespace vkr::core
