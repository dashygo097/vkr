#include <set>

#include "interface/device.hpp"
#include "interface/instance.hpp"
#include "interface/vk_utils.hpp"

Device::Device(VkInstance instance, VkSurfaceKHR surface,
               const std::vector<const char *> &deviceExtensions,
               const std::vector<const char *> &validationLayers)
    : instance(instance), surface(surface) {
  pickPhysicalDevice();
  createLogicalDevice(deviceExtensions, validationLayers);
}

Device::Device(const VulkanContext &ctx)
    : Device(ctx.instance, ctx.surface, ctx.deviceExtensions,
             ctx.validationLayers) {}

Device::~Device() {
  if (device != VK_NULL_HANDLE) {
    vkDestroyDevice(device, nullptr);
  }
  device = VK_NULL_HANDLE;
}

void Device::waitIdle() {
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }
}

void Device::pickPhysicalDevice() {
  {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &device : devices) {
      if (isSuitable(device)) {
        physicalDevice = device;
        break;
      }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("failed to find a suitable GPU!");
    }
  }
}

void Device::createLogicalDevice(std::vector<const char *> deviceExtensions,
                                 std::vector<const char *> validationLayers) {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

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

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

bool Device::isSuitable(VkPhysicalDevice pDevice) {
  QueueFamilyIndices indices = findQueueFamilies(pDevice, surface);
  return indices.isComplete();
}
