#include "vkr/core/instance.hh"
#include "vkr/logger.hh"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <vector>

namespace vkr::core {
Instance::Instance(InstanceDesc &desc) : desc_(desc) {
  VKR_CORE_INFO("Creating Vulkan Instance: {} ({}.{}.{})...", desc_.name,
                VK_VERSION_MAJOR(desc_.version),
                VK_VERSION_MINOR(desc_.version),
                VK_VERSION_PATCH(desc_.version));

  querySupport();
  resolveExtensions();
  resolveLayers();

#ifndef NDEBUG
  if (!hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    VKR_CORE_WARN("{} is not available; debug messenger disabled",
                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
#endif

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = desc_.name.c_str();
  appInfo.applicationVersion = desc_.version;
  appInfo.pEngineName = "vkr";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
#ifdef __APPLE__
  if (hasExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#else
  createInfo.flags = 0;
#endif

  std::vector<const char *> enabledExtensionNames{};
  enabledExtensionNames.reserve(enabled_extensions_.size());
  for (const auto &extension : enabled_extensions_) {
    enabledExtensionNames.push_back(extension.c_str());
  }

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(enabledExtensionNames.size());
  createInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifndef NDEBUG
  std::vector<const char *> enabledLayerNames{};
  enabledLayerNames.reserve(enabled_layers_.size());
  for (const auto &layer : enabled_layers_) {
    enabledLayerNames.push_back(layer.c_str());
  }

  createInfo.enabledLayerCount =
      static_cast<uint32_t>(enabledLayerNames.size());
  createInfo.ppEnabledLayerNames = enabledLayerNames.data();

  if (!enabled_layers_.empty() &&
      hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    DebugMessenger::populateCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  } else {
    createInfo.pNext = nullptr;
  }
#else
  createInfo.enabledLayerCount = 0;

  createInfo.pNext = nullptr;
#endif

  if (vkCreateInstance(&createInfo, nullptr, &vk_instance_) != VK_SUCCESS) {
    VKR_CORE_ERROR("failed to create instance!");
  }

#ifndef NDEBUG
  if (!enabled_layers_.empty() &&
      hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    debug_messenger_ = std::make_unique<DebugMessenger>(vk_instance_);
  }
#endif

  for (const auto &ext : enabled_extensions_) {
    VKR_CORE_TRACE("Enabled Extension: {}", ext);
  }
  for (const auto &layer : enabled_layers_) {
    VKR_CORE_TRACE("Enabled Layer: {}", layer);
  }

  VKR_CORE_INFO("Vulkan Instance created successfully.");
}

Instance::~Instance() {
#ifndef NDEBUG
  debug_messenger_.reset();
#endif
  vkDestroyInstance(vk_instance_, nullptr);
  vk_instance_ = VK_NULL_HANDLE;
}

void Instance::querySupport() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  available_extensions_.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         available_extensions_.data());

  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  available_layers_.resize(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, available_layers_.data());
}

void Instance::resolveExtensions() {
  enabled_extensions_.clear();

  if (desc_.surfaceIntegration == SurfaceIntegration::GLFW) {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
      const std::string extension{glfwExtensions[i]};
      if (!supportsExtension(extension)) {
        VKR_CORE_ERROR("Required GLFW instance extension is not available: {}",
                       extension);
      }

      if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                    extension) == enabled_extensions_.end()) {
        enabled_extensions_.push_back(extension);
      }
    }
  }

#ifdef __APPLE__
  const std::vector<std::string> platformRequiredExtensions{
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
  };

  for (const auto &extension : platformRequiredExtensions) {
    if (!supportsExtension(extension)) {
      VKR_CORE_ERROR("Required Apple instance extension is not available: {}",
                     extension);
    }

    if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                  extension) == enabled_extensions_.end()) {
      enabled_extensions_.push_back(extension);
    }
  }
#endif

  for (const auto &extension : desc_.requiredExtensions) {
    if (!supportsExtension(extension)) {
      VKR_CORE_ERROR("Required instance extension is not available: {}",
                     extension);
    }

    if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                  extension) == enabled_extensions_.end()) {
      enabled_extensions_.push_back(extension);
    }
  }

  for (const auto &extension : desc_.optionalExtensions) {
    if (!supportsExtension(extension)) {
      VKR_CORE_WARN("Optional instance extension is not available: {}",
                    extension);
      continue;
    }

    if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                  extension) == enabled_extensions_.end()) {
      enabled_extensions_.push_back(extension);
    }
  }

#ifndef NDEBUG
  if (supportsExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) &&
      std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME) ==
          enabled_extensions_.end()) {
    enabled_extensions_.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
#endif
}

void Instance::resolveLayers() {
  enabled_layers_.clear();

#ifndef NDEBUG
  const std::string validationLayer{"VK_LAYER_KHRONOS_validation"};
  if (supportsLayer(validationLayer)) {
    enabled_layers_.push_back(validationLayer);
  } else {
    VKR_CORE_WARN("validation layers requested, but not available; continuing "
                  "without validation layers");
  }
#endif
}

auto Instance::supportsExtension(const std::string &extension) const -> bool {
  for (const auto &available : available_extensions_) {
    if (extension == available.extensionName) {
      return true;
    }
  }

  return false;
}

auto Instance::supportsLayer(const std::string &layer) const -> bool {
  for (const auto &available : available_layers_) {
    if (layer == available.layerName) {
      return true;
    }
  }

  return false;
}

auto Instance::hasExtension(const std::string &extension) const noexcept
    -> bool {
  return std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                   extension) != enabled_extensions_.end();
}

auto Instance::hasLayer(const std::string &layer) const noexcept -> bool {
  return std::find(enabled_layers_.begin(), enabled_layers_.end(), layer) !=
         enabled_layers_.end();
}
} // namespace vkr::core
