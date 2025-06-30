#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

class Device {
public:
  Device(VkInstance instance, VkSurfaceKHR surface,
         std::vector<const char *> deviceExtensions,
         std::vector<const char *> validationLayers);
  ~Device();

  VkDevice getVkDevice() const;
  VkPhysicalDevice getVkPhysicalDevice() const;
  VkQueue getGraphicsQueue() const;
  VkQueue getPresentQueue() const;

private:
  VkInstance instance;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  bool isSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};
