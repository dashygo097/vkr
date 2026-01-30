#pragma once

#include "./command_pool.hh"
#include "./device.hh"

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace vkr::core {
class CommandBuffers {
public:
  explicit CommandBuffers(const Device &device, const CommandPool &commandPool);
  ~CommandBuffers();

  CommandBuffers(const CommandBuffers &) = delete;
  CommandBuffers &operator=(const CommandBuffers &) = delete;

  [[nodiscard]] std::vector<VkCommandBuffer> &commandBuffers() noexcept {
    return vk_command_buffers_;
  }
  [[nodiscard]] VkCommandBuffer &commandBuffer(uint32_t index) {
    return vk_command_buffers_.at(index);
  }

private:
  // dependencies
  const Device &device_;
  const CommandPool &commandPool_;

  // components
  std::vector<VkCommandBuffer> vk_command_buffers_{};
};

} // namespace vkr::core
