#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/queue_families.hh"
#include "vkr/logger.hh"
#include <set>

namespace vkr::core {
Device::Device(const Instance &instance, const Surface &surface,
               const std::vector<const char *> &deviceExtensions,
               const std::vector<const char *> &validationLayers)
    : instance_(instance), surface_(surface) {
  VKR_CORE_INFO("Creating logical device...");

  pickPhysicalDevice();

  createLogicalDevice(deviceExtensions, validationLayers);

  for (const auto &ext : deviceExtensions) {
    VKR_CORE_TRACE("Enabled Extension: {}", ext);
  }

  VKR_CORE_INFO("Logical device created successfully.");
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
  VKR_CORE_INFO("Found {} Vulkan-compatible device(s): ", deviceCount);

  if (deviceCount == 0) {
    VKR_CORE_ERROR("Failed to find Devices with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance_.instance(), &deviceCount,
                             devices.data());

  for (const auto &device : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VKR_CORE_INFO("--- Device FOUND: {} ---", deviceProperties.deviceName);
    VKR_CORE_TRACE("  -- API Version: {}.{}.{}",
                   VK_VERSION_MAJOR(deviceProperties.apiVersion),
                   VK_VERSION_MINOR(deviceProperties.apiVersion),
                   VK_VERSION_PATCH(deviceProperties.apiVersion));
    VKR_CORE_TRACE("  -- Driver Version: {}.{}.{}",
                   VK_VERSION_MAJOR(deviceProperties.driverVersion),
                   VK_VERSION_MINOR(deviceProperties.driverVersion),
                   VK_VERSION_PATCH(deviceProperties.driverVersion));
    VKR_CORE_TRACE("  -- Vendor ID: {:#06x}", deviceProperties.vendorID);
    VKR_CORE_TRACE("  -- Device ID: {:#06x}", deviceProperties.deviceID);

    if (isSuitable(device)) {
      vk_physical_device_ = device;
      VKR_CORE_INFO("Selected device: {}", deviceProperties.deviceName);
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
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily(),
                                            indices.presentFamily()};

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
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#ifndef NDEBUG
  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();
#else
  createInfo.enabledLayerCount = 0;
#endif

  if (vkCreateDevice(vk_physical_device_, &createInfo, nullptr,
                     &vk_logical_device_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create logical device!");
  }

  vkGetDeviceQueue(vk_logical_device_, indices.graphicsFamily(), 0,
                   &vk_graphics_queue_);
  vkGetDeviceQueue(vk_logical_device_, indices.presentFamily(), 0,
                   &vk_present_queue_);
}

bool Device::isSuitable(VkPhysicalDevice physicalDevice) {
  QueueFamilyIndices indices(surface_, physicalDevice);
  return indices.isComplete();
}

} // namespace vkr::core
