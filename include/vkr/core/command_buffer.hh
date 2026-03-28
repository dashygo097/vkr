#pragma once

#include "./command_pool.hh"
#include "./device.hh"

namespace vkr::core {
static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class CommandBuffers {
public:
  explicit CommandBuffers(const Device &device, const CommandPool &commandPool);
  ~CommandBuffers();

  CommandBuffers(const CommandBuffers &) = delete;
  auto operator=(const CommandBuffers &) -> CommandBuffers & = delete;

  [[nodiscard]] auto commandBuffers() noexcept -> std::vector<VkCommandBuffer> & {
    return vk_command_buffers_;
  }
  [[nodiscard]] auto commandBuffer(uint32_t index) -> VkCommandBuffer & {
    return vk_command_buffers_.at(index);
  }

private:
  // dependencies
  const Device &device_;
  const CommandPool &command_pool_;

  // components
  std::vector<VkCommandBuffer> vk_command_buffers_{};
};

} // namespace vkr::core
