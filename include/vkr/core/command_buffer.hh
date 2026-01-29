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
    return _commandBuffers;
  }
  [[nodiscard]] VkCommandBuffer &commandBuffer(uint32_t index) noexcept {
    return _commandBuffers[index];
  }

private:
  // dependencies
  const Device &device;
  const CommandPool &commandPool;

  // components
  std::vector<VkCommandBuffer> _commandBuffers{};
};

} // namespace vkr::core
