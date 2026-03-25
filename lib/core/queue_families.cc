#include "vkr/core/queue_families.hh"
#include "vkr/logger.hh"

namespace vkr::core {
QueueFamilyIndices::QueueFamilyIndices(const VkPhysicalDevice &physicalDevice,
                                       const Surface &surface)
    : surface_(surface), vk_physical_device_(physicalDevice) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_,
                                           &queueFamilyCount, nullptr);
  if (!queueFamilyCount) {
    VKR_CORE_ERROR("No queue families found on the physical device.");
  }

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      vk_physical_device_, &queueFamilyCount, queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilies.size(); i++) {
    const auto &queueFamily = queueFamilies[i];
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphics_family_ = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device_, i,
                                         surface.surface(), &presentSupport);
    if (presentSupport) {
      present_family_ = i;
    }
    if (isComplete()) {
      break;
    }
  }
}

} // namespace vkr::core
