#include "vkr/core/core_utils.hh"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>

namespace vkr::core {
auto getRequiredExtensions(const std::vector<const char *> &preExtensions)
    -> std::vector<const char *> {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  auto appendUnique = [&extensions](const char *extension) {
    if (std::find(extensions.begin(), extensions.end(), extension) ==
        extensions.end()) {
      extensions.push_back(extension);
    }
  };

  for (const auto *extension : preExtensions) {
    appendUnique(extension);
  }

#ifndef NDEBUG
  appendUnique(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

auto checkValidationLayerSupport(
    const std::vector<const char *> &validationLayers) -> bool {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
};

} // namespace vkr::core
