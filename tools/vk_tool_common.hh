#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <vulkan/vulkan_beta.h>
#endif

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace vktool {

inline auto versionString(uint32_t version) -> std::string {
  std::ostringstream out{};
  out << VK_VERSION_MAJOR(version) << "." << VK_VERSION_MINOR(version) << "."
      << VK_VERSION_PATCH(version);
  return out.str();
}

inline auto instanceExtensions() -> std::vector<VkExtensionProperties> {
  uint32_t count = 0;
  if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to enumerate instance extension count");
  }

  std::vector<VkExtensionProperties> extensions(count);
  if (vkEnumerateInstanceExtensionProperties(nullptr, &count,
                                             extensions.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to enumerate instance extensions");
  }

  return extensions;
}

inline auto hasExtension(const std::vector<VkExtensionProperties> &extensions,
                         const char *name) -> bool {
  return std::any_of(extensions.begin(), extensions.end(),
                     [name](const VkExtensionProperties &extension) {
                       return std::strcmp(extension.extensionName, name) == 0;
                     });
}

inline auto createInstance(const char *appName, bool requireSurface)
    -> VkInstance {
  const auto available = instanceExtensions();

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "None";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  std::vector<const char *> extensions{};
  auto appendUnique = [&extensions](const char *extension) {
    if (std::find_if(extensions.begin(), extensions.end(),
                     [extension](const char *enabled) {
                       return std::strcmp(enabled, extension) == 0;
                     }) == extensions.end()) {
      extensions.push_back(extension);
    }
  };

  if (requireSurface) {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions == nullptr || glfwExtensionCount == 0) {
      throw std::runtime_error("GLFW did not return required Vulkan "
                               "surface extensions");
    }

    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
      appendUnique(glfwExtensions[i]);
    }
  }

#ifdef __APPLE__
  if (hasExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    appendUnique(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  }
  if (hasExtension(available,
                   VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
    appendUnique(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
#endif

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames =
      extensions.empty() ? nullptr : extensions.data();

#ifdef __APPLE__
  if (hasExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#endif

  VkInstance instance = VK_NULL_HANDLE;
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create Vulkan instance");
  }

  return instance;
}

inline auto physicalDevices(VkInstance instance)
    -> std::vector<VkPhysicalDevice> {
  uint32_t count = 0;
  if (vkEnumeratePhysicalDevices(instance, &count, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("failed to enumerate physical device count");
  }

  std::vector<VkPhysicalDevice> devices(count);
  if (count > 0 &&
      vkEnumeratePhysicalDevices(instance, &count, devices.data()) !=
          VK_SUCCESS) {
    throw std::runtime_error("failed to enumerate physical devices");
  }

  return devices;
}

inline auto queueFlagsString(VkQueueFlags flags) -> std::string {
  std::ostringstream out{};
  if (flags & VK_QUEUE_GRAPHICS_BIT) {
    out << " graphics";
  }
  if (flags & VK_QUEUE_COMPUTE_BIT) {
    out << " compute";
  }
  if (flags & VK_QUEUE_TRANSFER_BIT) {
    out << " transfer";
  }
  if (flags & VK_QUEUE_SPARSE_BINDING_BIT) {
    out << " sparse";
  }
  if (flags == 0) {
    out << " none";
  }
  return out.str();
}

} // namespace vktool
