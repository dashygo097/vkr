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
    return vk_messenger_;
  }

private:
  // dependencies
  VkInstance instance_{VK_NULL_HANDLE};

  // components
  VkDebugUtilsMessengerEXT vk_messenger_{VK_NULL_HANDLE};

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
};
} // namespace vkr::core
