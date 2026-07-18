#pragma once

#include "vkr/core/device.hh"

namespace vkr::core {

enum class CommandQueueRole {
  Graphics,
  Compute,
  Transfer,
};

struct CommandPoolDesc {
  CommandQueueRole queueRole{CommandQueueRole::Graphics};
  VkCommandPoolCreateFlags flags{
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    switch (queueRole) {
    case CommandQueueRole::Graphics:
    case CommandQueueRole::Compute:
    case CommandQueueRole::Transfer:
      return true;
    }

    return false;
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("queueRole", queueRole);
    ar("flags", flags);
  }
};

class CommandPool {
public:
  explicit CommandPool(const Device &device, CommandPoolDesc &desc);
  ~CommandPool();

  CommandPool(const CommandPool &) = delete;
  auto operator=(const CommandPool &) -> CommandPool & = delete;

  [[nodiscard]] auto commandPool() const noexcept -> VkCommandPool {
    return vk_command_pool_;
  }
  [[nodiscard]] auto queueRole() const noexcept -> CommandQueueRole {
    return desc_.queueRole;
  }
  [[nodiscard]] auto queueFamily() const noexcept -> uint32_t {
    return queue_family_;
  }
  [[nodiscard]] auto queue() const noexcept -> VkQueue { return queue_; }

private:
  // dependencies
  const Device &device_;
  CommandPoolDesc &desc_;

  // components
  VkCommandPool vk_command_pool_{VK_NULL_HANDLE};
  uint32_t queue_family_{VK_QUEUE_FAMILY_IGNORED};
  VkQueue queue_{VK_NULL_HANDLE};

  void resolveQueue();
};
} // namespace vkr::core
