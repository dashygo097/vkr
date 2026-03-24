#include "vkr/core/queue_families.hh"
#include "vkr/logger.hh"

namespace vkr::core {
QueueFamilyIndices::QueueFamilyIndices(const VkPhysicalDevice &physicalDevice,
                                       const Surface &surface,
                                       bool enableGraphics, bool enablePresent,
                                       bool enableCompute)
    : surface_(surface), vk_physical_device_(physicalDevice),
      enable_graphics_(enableGraphics), enable_compute_(enableCompute) {
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
    if (enableGraphics && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      graphics_family_ = i;
    }
    if (enableCompute && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      compute_family_ = i;
    }
    if (enablePresent) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device_, i,
                                           surface.surface(), &presentSupport);
      if (presentSupport) {
        present_family_ = i;
      }
    }
    if (isComplete()) {
      break;
    }
  }
}

} // namespace vkr::core
