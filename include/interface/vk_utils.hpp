#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pDevice,
                                     VkSurfaceKHR surface);

std::vector<const char *>
getRequiredExtensions(std::vector<const char *> preExtensions);
bool checkValidationLayerSupport(std::vector<const char *> validationLayers);
