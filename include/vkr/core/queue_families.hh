#pragma once

#include <vulkan/vulkan.h>

namespace vkr::core {

class QueueFamilyIndices {
public:
  explicit QueueFamilyIndices(const VkSurfaceKHR &surface,
                              const VkPhysicalDevice &physicalDevice);
  ~QueueFamilyIndices() = default;

  [[nodiscard]] auto isComplete() const -> bool {
    bool graphicsComplete = graphics_family_ != VK_QUEUE_FAMILY_IGNORED;
    bool presentComplete = present_family_ != VK_QUEUE_FAMILY_IGNORED;
    return graphicsComplete && presentComplete;
  }

  [[nodiscard]] auto graphicsFamily() const -> uint32_t {
    return graphics_family_;
  }
  [[nodiscard]] auto presentFamily() const -> uint32_t {
    return present_family_;
  }

private:
  // dependencies
  const VkSurfaceKHR &surface_;
  const VkPhysicalDevice &vk_physical_device_;

  // components
  uint32_t graphics_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t present_family_{VK_QUEUE_FAMILY_IGNORED};
};

} // namespace vkr::core
