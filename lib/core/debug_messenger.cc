#include "vkr/core/debug_messenger.hh"
#include "vkr/logger.hh"
#include <iostream>

namespace vkr::core {
DebugMessenger::DebugMessenger(VkInstance instance) : instance_(instance) {
  VKR_CORE_INFO("Creating Debug Messenger...");

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateCreateInfo(createInfo);

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");

  if (func &&
      func(instance, &createInfo, nullptr, &vk_messenger_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create debug messenger!");
  }

  VKR_CORE_INFO("Debug Messenger created successfully.");
}

DebugMessenger::~DebugMessenger() {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance_, "vkDestroyDebugUtilsMessengerEXT");

  if (func && vk_messenger_ != VK_NULL_HANDLE) {
    func(instance_, vk_messenger_, nullptr);
  }
  vk_messenger_ = VK_NULL_HANDLE;
}

void DebugMessenger::populateCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    VKR_CORE_ERROR("{}", pCallbackData->pMessage);
  } else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    VKR_CORE_WARN("{}", pCallbackData->pMessage);
  } else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    VKR_CORE_INFO("{}", pCallbackData->pMessage);
  } else {
    VKR_CORE_TRACE("{}", pCallbackData->pMessage);
  }

  return VK_FALSE;
}
} // namespace vkr::core
