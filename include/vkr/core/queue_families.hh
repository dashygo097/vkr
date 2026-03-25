#pragma once

#include "./surface.hh"

namespace vkr::core {

class QueueFamilyIndices {
public:
  // NOTE: All enabled for now
  explicit QueueFamilyIndices(const VkPhysicalDevice &physicalDevice,
                              const Surface &surface);
  ~QueueFamilyIndices() = default;

  [[nodiscard]] bool isComplete() const {
    bool graphicsComplete = graphics_family_ != VK_QUEUE_FAMILY_IGNORED;
    bool presentComplete = present_family_ != VK_QUEUE_FAMILY_IGNORED;
    return graphicsComplete && presentComplete;
  }

  [[nodiscard]] uint32_t graphicsFamily() const { return graphics_family_; }
  [[nodiscard]] uint32_t presentFamily() const { return present_family_; }

private:
  // dependencies
  const Surface &surface_;
  const VkPhysicalDevice &vk_physical_device_;

  // components
  uint32_t graphics_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t present_family_{VK_QUEUE_FAMILY_IGNORED};
};

} // namespace vkr::core
