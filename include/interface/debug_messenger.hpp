#pragma once

#include <vulkan/vulkan.h>

#include "ctx.hpp"

class DebugMessenger {
public:
  DebugMessenger(VkInstance instance);
  DebugMessenger(const VulkanContext &ctx);
  ~DebugMessenger();

  static void
  populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

private:
  // dependencies
  VkInstance instance;

  // components
  VkDebugUtilsMessengerEXT messenger{VK_NULL_HANDLE};

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
};
