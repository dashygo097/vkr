#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <set>
#include <vector>
#include <vulkan/vulkan_core.h>

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

    VkPhysicalDeviceProperties physicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

    VKR_CORE_INFO("--- Device FOUND: {} ---",
                  physicalDeviceProperties.deviceName);
    VKR_CORE_TRACE("  -- Supported Vulkan API version: {}.{}.{}",
                   VK_API_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
                   VK_API_VERSION_MINOR(physicalDeviceProperties.apiVersion),
                   VK_API_VERSION_PATCH(physicalDeviceProperties.apiVersion));
    VKR_CORE_TRACE("  -- Driver Version: {}.{}.{}",
                   VK_VERSION_MAJOR(physicalDeviceProperties.driverVersion),
                   VK_VERSION_MINOR(physicalDeviceProperties.driverVersion),
                   VK_VERSION_PATCH(physicalDeviceProperties.driverVersion));
    VKR_CORE_TRACE("  -- Vendor ID: {:#06x}",
                   physicalDeviceProperties.vendorID);
    VKR_CORE_TRACE("  -- Device ID: {:#06x}",
                   physicalDeviceProperties.deviceID);

    if (VK_API_VERSION_VARIANT(physicalDeviceProperties.apiVersion) != 0 ||
        physicalDeviceProperties.apiVersion < VK_API_VERSION_1_2) {
      VKR_CORE_WARN("Skipping device without Vulkan 1.2 support: {}",
                    physicalDeviceProperties.deviceName);
      continue;
    }
    const auto apiVersion =
        std::min(instance_.apiVersion(), physicalDeviceProperties.apiVersion);

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

    if (!resolveExtensions(apiVersion)) {
      VKR_CORE_WARN("Skipping device with missing required extension(s): {}",
                    physicalDeviceProperties.deviceName);
      continue;
    }

    // NOTE: explicitly integrate dynamic rendering support
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
    dynamicRendering.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &dynamicRendering;
    vkGetPhysicalDeviceFeatures2(device, &features);
    if (!dynamicRendering.dynamicRendering) {
      VKR_CORE_WARN("Skipping device without the dynamicRendering feature: {}",
                    physicalDeviceProperties.deviceName);
      continue;
    }

    vk_physical_device_ = device;
    physical_device_properties_ = physicalDeviceProperties;
    physical_device_features_ = features.features;
    api_version_ = apiVersion;
    VKR_CORE_INFO("Selected device: {}",
                  physical_device_properties_.deviceName);
    break;
  }

  if (vk_physical_device_ == VK_NULL_HANDLE) {
    VKR_CORE_ERROR("No suitable device supports Vulkan 1.2 or later, dynamic "
                   "rendering, and the required extensions");
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
  deviceFeatures.samplerAnisotropy =
      physical_device_features_.samplerAnisotropy;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
  dynamicRendering.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  dynamicRendering.dynamicRendering = VK_TRUE;
  createInfo.pNext = &dynamicRendering;

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

  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = nullptr;

  VkDevice logicalDevice{VK_NULL_HANDLE};
  const auto result =
      vkCreateDevice(vk_physical_device_, &createInfo, nullptr, &logicalDevice);
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create logical device. VkResult: {}",
                   static_cast<int>(result));
  }

  const bool coreRendering = api_version_ >= VK_API_VERSION_1_3;
  const auto beginRendering =
      reinterpret_cast<PFN_vkCmdBeginRendering>(vkGetDeviceProcAddr(
          logicalDevice,
          coreRendering ? "vkCmdBeginRendering" : "vkCmdBeginRenderingKHR"));
  const auto endRendering =
      reinterpret_cast<PFN_vkCmdEndRendering>(vkGetDeviceProcAddr(
          logicalDevice,
          coreRendering ? "vkCmdEndRendering" : "vkCmdEndRenderingKHR"));
  if (!beginRendering || !endRendering) {
    vkDestroyDevice(logicalDevice, nullptr);
    VKR_CORE_ERROR("Dynamic rendering entry points are unavailable for the {} "
                   "API path",
                   coreRendering ? "Vulkan 1.3+ core" : "KHR extension");
  }
  vk_logical_device_ = logicalDevice;
  cmd_begin_rendering_ = beginRendering;
  cmd_end_rendering_ = endRendering;
  VKR_CORE_INFO("Dynamic rendering enabled via {}",
                coreRendering ? "Vulkan 1.3+ core"
                              : "VK_KHR_dynamic_rendering");

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

auto Device::resolveExtensions(uint32_t apiVersion) -> bool {
  enabled_extensions_.clear();

  if (apiVersion < VK_API_VERSION_1_3) {
    if (!hasExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) {
      VKR_CORE_WARN("Vulkan 1.2 requires the {} extension",
                    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
      return false;
    }
    enabled_extensions_.emplace_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
  }

  const std::string portabilityExtension{"VK_KHR_portability_subset"};
  if (hasExtension(portabilityExtension)) {
    enabled_extensions_.push_back(portabilityExtension);
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
    if (queueFamily.queueCount == 0) {
      continue;
    }

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
