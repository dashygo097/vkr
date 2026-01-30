#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/queue_families.hh"
#include <set>

namespace vkr::core {
Device::Device(const Instance &instance, const Surface &surface,
               const std::vector<const char *> &deviceExtensions,
               const std::vector<const char *> &validationLayers)
    : instance_(instance), surface_(surface) {
  pickPhysicalDevice();
  createLogicalDevice(deviceExtensions, validationLayers);
}

Device::~Device() {
  if (vk_logical_device_ != VK_NULL_HANDLE) {
    vkDestroyDevice(vk_logical_device_, nullptr);
  }
  vk_logical_device_ = VK_NULL_HANDLE;
}

void Device::waitIdle() {
  if (vk_logical_device_ != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(vk_logical_device_);
  }
}

void Device::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance_.instance(), &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance_.instance(), &deviceCount,
                             devices.data());

  for (const auto &device : devices) {
    if (isSuitable(device)) {
      vk_physical_device_ = device;
      break;
    }
  }

  if (vk_physical_device_ == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void Device::createLogicalDevice(std::vector<const char *> deviceExtensions,
                                 std::vector<const char *> validationLayers) {
  QueueFamilyIndices indices(surface_, vk_physical_device_);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily().value(),
                                            indices.presentFamily().value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (ENABLE_VALIDATION_LAYERS) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(vk_physical_device_, &createInfo, nullptr,
                     &vk_logical_device_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(vk_logical_device_, indices.graphicsFamily().value(), 0,
                   &vk_graphics_queue_);
  vkGetDeviceQueue(vk_logical_device_, indices.presentFamily().value(), 0,
                   &vk_present_queue_);
}

bool Device::isSuitable(VkPhysicalDevice physicalDevice) {
  QueueFamilyIndices indices(surface_, physicalDevice);
  return indices.isComplete();
}

} // namespace vkr::core
