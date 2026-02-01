#include "vkr/core/instance.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/logger.hh"

namespace vkr::core {
Instance::Instance(const std::string appName, uint32_t appVersion,
                   const std::vector<const char *> &preExtensions,
                   const std::vector<const char *> &validationLayers) {
  VKR_CORE_INFO("Creating Vulkan Instance: {} ({}.{}.{})...", appName,
                VK_VERSION_MAJOR(appVersion), VK_VERSION_MINOR(appVersion),
                VK_VERSION_PATCH(appVersion));

#ifndef NDEBUG
  if (!checkValidationLayerSupport(validationLayers)) {
    VKR_CORE_ERROR("validation layers requested, but not available!");
  }
#endif

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName.c_str();
  appInfo.applicationVersion = appVersion;
  appInfo.pEngineName = "vkr";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  auto extensions = getRequiredExtensions(preExtensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifndef NDEBUG
  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();

  DebugMessenger::populateCreateInfo(debugCreateInfo);
  createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
#else
  createInfo.enabledLayerCount = 0;

  createInfo.pNext = nullptr;
#endif

  if (vkCreateInstance(&createInfo, nullptr, &vk_instance_) != VK_SUCCESS) {
    VKR_CORE_ERROR("failed to create instance!");
  }

#ifndef NDEBUG
  debug_messenger_ = std::make_unique<DebugMessenger>(vk_instance_);
#endif

  for (const auto &ext : extensions) {
    VKR_CORE_TRACE("Enabled Extension: {}", ext);
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
} // namespace vkr::core
