#include "vkr/core/instance.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/logger.hh"

namespace vkr::core {
Instance::Instance(InstanceDesc &desc) : desc_(desc) {
  VKR_CORE_INFO("Creating Vulkan Instance: {} ({}.{}.{})...", desc_.name,
                VK_VERSION_MAJOR(desc_.version),
                VK_VERSION_MINOR(desc_.version),
                VK_VERSION_PATCH(desc_.version));

#ifndef NDEBUG
  if (!checkValidationLayerSupport(desc_.validationLayers)) {
    VKR_CORE_ERROR("validation layers requested, but not available!");
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
  createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  auto extensions = getRequiredExtensions(desc_.extensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifndef NDEBUG
  createInfo.enabledLayerCount =
      static_cast<uint32_t>(desc_.validationLayers.size());
  createInfo.ppEnabledLayerNames = desc_.validationLayers.data();

  DebugMessenger::populateCreateInfo(debugCreateInfo);
  createInfo.pNext = &debugCreateInfo;
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
