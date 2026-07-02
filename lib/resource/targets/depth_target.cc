#include "vkr/resource/targets/depth_target.hh"
#include "vkr/logger.hh"
#include <vulkan/vulkan_core.h>

namespace vkr::resource {
DepthTarget::DepthTarget(const core::Device &device,
                         const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  image_ = std::make_unique<Image>(device_, command_pool_);
  image_view_ = std::make_unique<ImageView>(device_);
}

DepthTarget::~DepthTarget() { destory(); };

void DepthTarget::create() {
  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("DepthTarget has undefined format");
  }

  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("DepthTarget has invalid size: {}x{}", desc_.width,
                  desc_.height);
  }

  image_->create();
  image_view_->create();
}

void DepthTarget::destory() {
  image_view_->destroy();
  image_->destroy();
}

void DepthTarget::update(const DepthTargetDesc &desc) {
  desc_ = desc;
  image_desc_ =
      ImageDesc::depthAttachment(desc_.width, desc_.height, desc_.format);
  image_desc_.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("DepthTarget has undefined format");
  }

  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("DepthTarget has invalid size: {}x{}", desc_.width,
                  desc_.height);
  }

  image_->update(image_desc_);
  image_view_->update(ImageViewDesc::depth2D(image_->image(), desc_.format));
}

auto findSupportedFormat(VkPhysicalDevice physicalDevice,
                         const std::vector<VkFormat> &candidates,
                         VkImageTiling tiling, VkFormatFeatureFlags features)
    -> VkFormat {
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

auto findDepthFormat(VkPhysicalDevice physicalDevice) -> VkFormat {
  return findSupportedFormat(
      physicalDevice,
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace vkr::resource
