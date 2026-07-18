#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/logger.hh"
#include <set>
#include <vector>

namespace vkr::core {
Device::Device(const Instance &instance, const Surface &surface,
               DeviceDesc &deviceDesc)
    : instance_(instance), surface_(surface), desc_(deviceDesc) {
  VKR_CORE_INFO("Creating logical device...");

  pickPhysicalDevice();
  createLogicalDevice();

  for (const auto &ext : desc_.extensions) {
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

void Device::waitIdle() const {
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

    const auto support = queryQueueFamilySupport(device);

    VKR_CORE_TRACE("  -- Queue Families: {}", support.families.size());
    VKR_CORE_TRACE("  -- Graphics: {}",
                   support.supportsGraphics() ? "supported" : "unsupported");
    VKR_CORE_TRACE("  -- Present: {}",
                   support.supportsPresent() ? "supported" : "unsupported");
    VKR_CORE_TRACE("  -- Compute: {}",
                   support.supportsCompute() ? "supported" : "unsupported");
    VKR_CORE_TRACE("  -- Transfer: {}",
                   support.supportsTransfer() ? "supported" : "unsupported");

    if (support.supportsGraphicsPresentation()) {
      vk_physical_device_ = device;
      queue_family_support_ = support;
      VKR_CORE_INFO("Selected device: {}", deviceProperties.deviceName);
      break;
    }
  }

  if (vk_physical_device_ == VK_NULL_HANDLE) {
    VKR_CORE_ERROR("failed to find a suitable GPU!");
  }
}

void Device::createLogicalDevice() {
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      queue_family_support_.graphicsFamily,
      queue_family_support_.presentFamily,
  };

  if (queue_family_support_.supportsCompute()) {
    uniqueQueueFamilies.insert(queue_family_support_.computeFamily);
  }
  if (queue_family_support_.supportsTransfer()) {
    uniqueQueueFamilies.insert(queue_family_support_.transferFamily);
  }

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
      static_cast<uint32_t>(desc_.extensions.size());
  createInfo.ppEnabledExtensionNames = desc_.extensions.data();

#ifndef NDEBUG
  createInfo.enabledLayerCount =
      static_cast<uint32_t>(instance_.enabledLayerNames().size());
  createInfo.ppEnabledLayerNames = instance_.enabledLayerNames().data();
#else
  createInfo.enabledLayerCount = 0;
#endif

  if (vkCreateDevice(vk_physical_device_, &createInfo, nullptr,
                     &vk_logical_device_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create logical device!");
  }

  vkGetDeviceQueue(vk_logical_device_, queue_family_support_.graphicsFamily, 0,
                   &vk_graphics_queue_);
  vkGetDeviceQueue(vk_logical_device_, queue_family_support_.presentFamily, 0,
                   &vk_present_queue_);

  if (queue_family_support_.supportsCompute()) {
    vkGetDeviceQueue(vk_logical_device_, queue_family_support_.computeFamily, 0,
                     &vk_compute_queue_);
  }

  if (queue_family_support_.supportsTransfer()) {
    vkGetDeviceQueue(vk_logical_device_, queue_family_support_.transferFamily,
                     0, &vk_transfer_queue_);
  }
}

auto Device::queryQueueFamilySupport(VkPhysicalDevice device) const
    -> QueueFamilySupport {
  QueueFamilySupport support{};

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  if (!queueFamilyCount) {
    VKR_CORE_ERROR("No queue families found on the physical device.");
  }

  support.families.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           support.families.data());

  for (uint32_t i = 0; i < support.families.size(); i++) {
    const auto &queueFamily = support.families[i];

    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        !support.supportsGraphics()) {
      support.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_.surface(),
                                         &presentSupport);
    if (presentSupport && !support.supportsPresent()) {
      support.presentFamily = i;
    }

    if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      const bool dedicatedCompute =
          (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;
      if (!support.supportsCompute() || dedicatedCompute) {
        support.computeFamily = i;
      }
    }

    if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      const bool dedicatedTransfer =
          (queueFamily.queueFlags &
           (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0;
      if (!support.supportsTransfer() || dedicatedTransfer) {
        support.transferFamily = i;
      }
    }
  }

  return support;
}

} // namespace vkr::core
