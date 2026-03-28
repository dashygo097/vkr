#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::core {

static void check_vk_result(VkResult err) {
  if (err == 0) {
    return;
}
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0) {
    abort();
}
}

auto
getRequiredExtensions(const std::vector<const char *> &preExtensions) -> std::vector<const char *>;
auto checkValidationLayerSupport(
    const std::vector<const char *> &validationLayers) -> bool;
} // namespace vkr::core
