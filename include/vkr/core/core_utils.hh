#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::core {

static void check_vk_result(VkResult err) {
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}

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
} // namespace vkr::core
