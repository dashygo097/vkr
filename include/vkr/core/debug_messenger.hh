#pragma once

#include <vulkan/vulkan.h>

namespace vkr::core {

class DebugMessenger final {
public:
  explicit DebugMessenger(VkInstance instance);
  ~DebugMessenger();

  DebugMessenger(const DebugMessenger &) = delete;
  auto operator=(const DebugMessenger &) -> DebugMessenger & = delete;

  static void
  populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  [[nodiscard]] auto messenger() const -> VkDebugUtilsMessengerEXT {
    return vk_messenger_;
  }

private:
  // dependencies
  VkInstance instance_{VK_NULL_HANDLE};

  // components
  VkDebugUtilsMessengerEXT vk_messenger_{VK_NULL_HANDLE};

  static VKAPI_ATTR auto VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) -> VkBool32;
};
} // namespace vkr::core
