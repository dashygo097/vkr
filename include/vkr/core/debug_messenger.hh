#pragma once

#include <vulkan/vulkan.h>

namespace vkr::core {

class DebugMessenger {
public:
  explicit DebugMessenger(VkInstance instance);
  ~DebugMessenger();

  DebugMessenger(const DebugMessenger &) = delete;
  DebugMessenger &operator=(const DebugMessenger &) = delete;

  static void
  populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  [[nodiscard]] VkDebugUtilsMessengerEXT messenger() const {
    return _messenger;
  }

private:
  // dependencies
  VkInstance instance{VK_NULL_HANDLE};

  // components
  VkDebugUtilsMessengerEXT _messenger{VK_NULL_HANDLE};

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
};
} // namespace vkr::core
