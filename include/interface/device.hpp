#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Device {
public:
  Device(VkInstance instance, VkSurfaceKHR surface,
         std::vector<const char *> deviceExtensions,
         std::vector<const char *> validationLayers);
  ~Device();

  void waitIdle();

  VkDevice getVkDevice() const { return device; }
  VkPhysicalDevice getVkPhysicalDevice() const { return physicalDevice; }
  VkQueue getGraphicsQueue() const { return graphicsQueue; }
  VkQueue getPresentQueue() const { return presentQueue; }

private:
  // dependencies
  VkInstance instance;
  VkSurfaceKHR surface;

  // components
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkDevice device{VK_NULL_HANDLE};
  VkQueue graphicsQueue{VK_NULL_HANDLE};
  VkQueue presentQueue{VK_NULL_HANDLE};

  void pickPhysicalDevice();
  void createLogicalDevice(std::vector<const char *> deviceExtensions,
                           std::vector<const char *> validationLayers);

  bool isSuitable(VkPhysicalDevice pDevice);
};
