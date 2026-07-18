#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <set>
#include <vector>

namespace vkr::core {
Device::Device(const Instance &instance, DeviceDesc &deviceDesc)
    : instance_(instance), desc_(deviceDesc) {
  VKR_CORE_INFO("Creating logical device...");

  pickPhysicalDevice();
  createLogicalDevice();

  for (const auto &ext : enabled_extensions_) {
    VKR_CORE_TRACE("Enabled Extension: {}", ext);
  }

  VKR_CORE_INFO("Logical device created successfully.");
}

Device::Device(const Instance &instance, const Surface &surface,
               DeviceDesc &deviceDesc)
    : instance_(instance), surface_(std::cref(surface)), desc_(deviceDesc) {
  VKR_CORE_INFO("Creating logical device...");

  pickPhysicalDevice();
  createLogicalDevice();

  for (const auto &ext : enabled_extensions_) {
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
    available_extensions_.clear();
    enabled_extensions_.clear();
    queue_families_.clear();
    graphics_family_ = VK_QUEUE_FAMILY_IGNORED;
    present_family_ = VK_QUEUE_FAMILY_IGNORED;
    compute_family_ = VK_QUEUE_FAMILY_IGNORED;
    transfer_family_ = VK_QUEUE_FAMILY_IGNORED;

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

    queryDeviceSupport(device);
    resolveQueueFamilies(device);

    VKR_CORE_TRACE("  -- Queue Families: {}", queue_families_.size());
    VKR_CORE_TRACE("  -- Graphics: {}",
                   supportsGraphics() ? "supported" : "unsupported");
    VKR_CORE_TRACE("  -- Present: {}",
                   supportsPresent() ? "supported" : "unsupported");
    VKR_CORE_TRACE("  -- Compute: {}",
                   supportsCompute() ? "supported" : "unsupported");
    VKR_CORE_TRACE("  -- Transfer: {}",
                   supportsTransfer() ? "supported" : "unsupported");

    if (!resolveExtensions()) {
      VKR_CORE_WARN("Skipping device with missing required extension(s): {}",
                    deviceProperties.deviceName);
      continue;
    }

    vk_physical_device_ = device;
    VKR_CORE_INFO("Selected device: {}", deviceProperties.deviceName);
    break;
  }

  if (vk_physical_device_ == VK_NULL_HANDLE) {
    VKR_CORE_ERROR("failed to find a suitable GPU!");
  }
}

void Device::createLogicalDevice() {
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies{};

  if (supportsGraphics()) {
    uniqueQueueFamilies.insert(graphics_family_);
  }
  if (supportsPresent()) {
    uniqueQueueFamilies.insert(present_family_);
  }
  if (supportsCompute()) {
    uniqueQueueFamilies.insert(compute_family_);
  }
  if (supportsTransfer()) {
    uniqueQueueFamilies.insert(transfer_family_);
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

  std::vector<const char *> enabledExtensionNames{};
  enabledExtensionNames.reserve(enabled_extensions_.size());
  for (const auto &extension : enabled_extensions_) {
    enabledExtensionNames.push_back(extension.c_str());
  }

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(enabledExtensionNames.size());
  createInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

#ifndef NDEBUG
  std::vector<const char *> enabledLayerNames{};
  enabledLayerNames.reserve(instance_.enabledLayers().size());
  for (const auto &layer : instance_.enabledLayers()) {
    enabledLayerNames.push_back(layer.c_str());
  }

  createInfo.enabledLayerCount =
      static_cast<uint32_t>(enabledLayerNames.size());
  createInfo.ppEnabledLayerNames = enabledLayerNames.data();
#else
  createInfo.enabledLayerCount = 0;
#endif

  if (vkCreateDevice(vk_physical_device_, &createInfo, nullptr,
                     &vk_logical_device_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create logical device!");
  }

  if (supportsGraphics()) {
    vkGetDeviceQueue(vk_logical_device_, graphics_family_, 0,
                     &vk_graphics_queue_);
  }
  if (supportsPresent()) {
    vkGetDeviceQueue(vk_logical_device_, present_family_, 0,
                     &vk_present_queue_);
  }

  if (supportsCompute()) {
    vkGetDeviceQueue(vk_logical_device_, compute_family_, 0,
                     &vk_compute_queue_);
  }

  if (supportsTransfer()) {
    vkGetDeviceQueue(vk_logical_device_, transfer_family_, 0,
                     &vk_transfer_queue_);
  }
}

void Device::queryDeviceSupport(VkPhysicalDevice device) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);
  available_extensions_.resize(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       available_extensions_.data());

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  if (!queueFamilyCount) {
    VKR_CORE_ERROR("No queue families found on the physical device.");
  }

  queue_families_.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queue_families_.data());
}

auto Device::resolveExtensions() -> bool {
  enabled_extensions_.clear();

  const std::vector<std::string> platformRequiredExtensions{
#ifdef __APPLE__
      VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
  };

  for (const auto &extension : platformRequiredExtensions) {
    if (!hasExtension(extension)) {
      VKR_CORE_WARN("Required device extension is not available: {}",
                    extension);
      return false;
    }

    if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                  extension) == enabled_extensions_.end()) {
      enabled_extensions_.push_back(extension);
    }
  }

  if (supportsPresent()) {
    const std::string extension{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (!hasExtension(extension)) {
      VKR_CORE_WARN("Required device extension is not available: {}",
                    extension);
      return false;
    }

    if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                  extension) == enabled_extensions_.end()) {
      enabled_extensions_.push_back(extension);
    }
  }

  for (const auto &extension : desc_.requiredExtensions) {
    if (!hasExtension(extension)) {
      VKR_CORE_WARN("Required device extension is not available: {}",
                    extension);
      return false;
    }

    if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                  extension) == enabled_extensions_.end()) {
      enabled_extensions_.push_back(extension);
    }
  }

  for (const auto &extension : desc_.optionalExtensions) {
    if (!hasExtension(extension)) {
      VKR_CORE_WARN("Optional device extension is not available: {}",
                    extension);
      continue;
    }

    if (std::find(enabled_extensions_.begin(), enabled_extensions_.end(),
                  extension) == enabled_extensions_.end()) {
      enabled_extensions_.push_back(extension);
    }
  }

  return true;
}

void Device::resolveQueueFamilies(VkPhysicalDevice device) {
  for (uint32_t i = 0; i < queue_families_.size(); i++) {
    const auto &queueFamily = queue_families_[i];

    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        !supportsGraphics()) {
      graphics_family_ = i;
    }

    if (surface_.has_value()) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_->get().surface(),
                                           &presentSupport);
      if (presentSupport && !supportsPresent()) {
        present_family_ = i;
      }
    }

    if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      const bool dedicatedCompute =
          (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;
      if (!supportsCompute() || dedicatedCompute) {
        compute_family_ = i;
      }
    }

    if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      const bool dedicatedTransfer =
          (queueFamily.queueFlags &
           (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0;
      if (!supportsTransfer() || dedicatedTransfer) {
        transfer_family_ = i;
      }
    }
  }
}

auto Device::hasExtension(const std::string &extension) const noexcept -> bool {
  for (const auto &available : available_extensions_) {
    if (extension == available.extensionName) {
      return true;
    }
  }

  return false;
}

} // namespace vkr::core
