#include "interface/instance.hpp"
#include "interface/extensions.hpp"
#include "interface/validation_layers.hpp"
#include <stdexcept>

Instance::Instance(VkApplicationInfo appInfo,
                   std::vector<const char *> validationLayers,
                   std::vector<const char *> preExtensions) {

  if (ENABLE_VALIDATION_LAYERS &&
      !checkValidationLayerSupport(validationLayers)) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

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

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

Instance::~Instance() {
  vkDestroyInstance(instance, nullptr);
  DestroyDebugUtilsMessengerEXT(nullptr);
}

VkInstance Instance::getVkInstance() const { return instance; }
