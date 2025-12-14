#include "vkr/core/instance.hh"
#include "vkr/core/utils.hh"

namespace vkr {
Instance::Instance(const std::string appName, const std::string engineName,
                   uint32_t appVersion, uint32_t engineVersion,
                   const std::vector<const char *> &preExtensions,
                   const std::vector<const char *> &validationLayers) {

  if (ENABLE_VALIDATION_LAYERS &&
      !checkValidationLayerSupport(validationLayers)) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName.c_str();
  appInfo.applicationVersion = appVersion;
  appInfo.pEngineName = engineName.c_str();
  appInfo.engineVersion = engineVersion;
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  auto extensions = getRequiredExtensions(preExtensions);
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (ENABLE_VALIDATION_LAYERS) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    DebugMessenger::populateCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }

  if (ENABLE_VALIDATION_LAYERS) {
    _debugMessenger = std::make_unique<DebugMessenger>(_instance);
  }
}

Instance::Instance(const VulkanContext &ctx)
    : Instance(ctx.appName, ctx.engineName, ctx.appVersion, ctx.engineVersion,
               ctx.preExtensions, ctx.validationLayers) {}

Instance::~Instance() {
  if (ENABLE_VALIDATION_LAYERS) {
    _debugMessenger.reset();
  }
  vkDestroyInstance(_instance, nullptr);
  _instance = VK_NULL_HANDLE;
}
} // namespace vkr
