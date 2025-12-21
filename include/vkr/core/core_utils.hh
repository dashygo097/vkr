#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr {

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice,
                                     VkSurfaceKHR surface);

std::vector<const char *>
getRequiredExtensions(const std::vector<const char *> &preExtensions);
bool checkValidationLayerSupport(
    const std::vector<const char *> &validationLayers);
} // namespace vkr
