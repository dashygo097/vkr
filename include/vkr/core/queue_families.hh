#pragma once

#include "./surface.hh"

namespace vkr::core {

class QueueFamilyIndices {
public:
  explicit QueueFamilyIndices(const Surface &surface,
                              const VkPhysicalDevice &physicalDevice,
                              bool enableGraphics = true,
                              bool enableCompute = false);
  ~QueueFamilyIndices() = default;

  [[nodiscard]] bool isComplete() const {
    bool graphicsComplete =
        !enable_graphics_ || graphics_family_ != VK_QUEUE_FAMILY_IGNORED;
    bool computeComplete =
        !enable_compute_ || compute_family_ != VK_QUEUE_FAMILY_IGNORED;
    bool presentComplete = present_family_ != VK_QUEUE_FAMILY_IGNORED;
    return graphicsComplete && computeComplete && presentComplete;
  }

  [[nodiscard]] uint32_t graphicsFamily() const { return graphics_family_; }
  [[nodiscard]] uint32_t presentFamily() const { return present_family_; }
  [[nodiscard]] uint32_t computeFamily() const { return compute_family_; }

private:
  // dependencies
  const Surface &surface_;
  const VkPhysicalDevice &vk_physical_device_;

  // components
  uint32_t graphics_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t present_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t compute_family_{VK_QUEUE_FAMILY_IGNORED};
  bool enable_graphics_{false};
  bool enable_compute_{false};
};

} // namespace vkr::core
