#include "vkr/core/queue_families.hh"

namespace vkr::core {
QueueFamilyIndices::QueueFamilyIndices(const Surface &surface,
                                       const VkPhysicalDevice &physicalDevice)
    : surface_(surface), vk_physical_device_(physicalDevice) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphics_family_ = i;
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
    i++;
  }
}

} // namespace vkr::core
