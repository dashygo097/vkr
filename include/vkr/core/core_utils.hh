#pragma once

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

std::vector<const char *>
getRequiredExtensions(const std::vector<const char *> &preExtensions);
bool checkValidationLayerSupport(
    const std::vector<const char *> &validationLayers);
} // namespace vkr::core
