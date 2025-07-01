#pragma once

#include <vector>
#include <vulkan/vulkan.h>

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
  // dependencies
  VkInstance instance;
  VkSurfaceKHR surface;

  // components
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  void pickPhysicalDevice();
  void createLogicalDevice(std::vector<const char *> deviceExtensions,
                           std::vector<const char *> validationLayers);

  bool isSuitable(VkPhysicalDevice pDevice);
};
