#pragma once

#include <vulkan/vulkan.h>

#include "../ctx.hpp"

namespace vkr {

class DebugMessenger {
public:
  DebugMessenger(VkInstance instance);
  DebugMessenger(const VulkanContext &ctx);
  ~DebugMessenger();

  DebugMessenger(const DebugMessenger &) = delete;
  DebugMessenger &operator=(const DebugMessenger &) = delete;

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
} // namespace vkr
