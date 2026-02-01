#include "vkr/resources/depth_resources.hh"
#include "vkr/logger.hh"

namespace vkr::resource {
DepthResources::DepthResources(const core::Device &device,
                               const core::Swapchain &swapchain,
                               const core::CommandPool &commandPool)
    : device_(device), swapchain_(swapchain), command_pool_(commandPool) {
  image_ = std::make_unique<Image>(device_, command_pool_);
  imageview_ = std::make_unique<ImageView>(device_);
  image_->create(swapchain_.extent2D().width, swapchain_.extent2D().height,
                 findDepthFormat(device_.physicalDevice()),
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  imageview_->create(*image_, findDepthFormat(device_.physicalDevice()),
                     VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
                             const std::vector<VkFormat> &candidates,
                             VkImageTiling tiling,
                             VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  VKR_RES_ERROR("failed to find supported format!");
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
  return findSupportedFormat(
      physicalDevice,
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace vkr::resource
