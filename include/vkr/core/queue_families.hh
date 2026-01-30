#pragma once

#include "./surface.hh"

namespace vkr::core {

class QueueFamilyIndices {
public:
  explicit QueueFamilyIndices(const Surface &surface,
                              const VkPhysicalDevice &physicalDevice);
  ~QueueFamilyIndices() = default;

  [[nodiscard]] bool isComplete() {
    return graphics_family_.has_value() && present_family_.has_value();
  }
  [[nodiscard]] std::optional<uint32_t> graphicsFamily() const {
    return graphics_family_;
  }
  [[nodiscard]] std::optional<uint32_t> presentFamily() const {
    return present_family_;
  }

private:
  // dependencies
  const Surface &surface_;
  const VkPhysicalDevice &vk_physical_device_;

  // components
  std::optional<uint32_t> graphics_family_;
  std::optional<uint32_t> present_family_;
};

} // namespace vkr::core
