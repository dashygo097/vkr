#include "vkr/core/queue_families.hh"
#include "vkr/logger.hh"

namespace vkr::core {
QueueFamilyIndices::QueueFamilyIndices(const Surface &surface,
                                       const VkPhysicalDevice &physicalDevice,
                                       bool enableGraphics, bool enableCompute)
    : surface_(surface), vk_physical_device_(physicalDevice),
      enable_graphics_(enableGraphics), enable_compute_(enableCompute) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  if (!queueFamilyCount) {
    VKR_CORE_ERROR("No queue families found on the physical device.");
  }

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilies.size(); i++) {
    const auto &queueFamily = queueFamilies[i];
    if (enableGraphics && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      graphics_family_ = i;
    }
    if (enableCompute && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      compute_family_ = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface.surface(),
                                         &presentSupport);
    if (presentSupport) {
      present_family_ = i;
    }
    if (isComplete()) {
      break;
    }
  }
}

} // namespace vkr::core
