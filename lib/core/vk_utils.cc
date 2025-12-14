#include "vkr/core/instance.hh"
#include "vkr/core/utils.hh"

namespace vkr {
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pDevice,
                                     VkSurfaceKHR surface) {
  QueueFamilyIndices indices;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount,
                                           queueFamilies.data());
  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }
    if (indices.isComplete()) {
      break;
    }
    i++;
  }
  return indices;
}

std::vector<const char *>
getRequiredExtensions(const std::vector<const char *> &preExtensions) {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  if (ENABLE_VALIDATION_LAYERS) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

bool checkValidationLayerSupport(
    const std::vector<const char *> &validationLayers) {
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
} // namespace vkr
