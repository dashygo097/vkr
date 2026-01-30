#pragma once

#include "../../core/device.hh"
#include "../../core/swapchain.hh"

namespace vkr::resources {
class DepthResources {
public:
  explicit DepthResources(const core::Device &device,
                          const core::Swapchain &swapChain);
  ~DepthResources();

  DepthResources(const DepthResources &) = delete;
  DepthResources &operator=(const DepthResources &) = delete;

  [[nodiscard]] VkImage image() const noexcept { return vk_image_; }
  [[nodiscard]] VkDeviceMemory imageMemory() const noexcept {
    return vk_memory_;
  }
  [[nodiscard]] VkImageView imageView() const noexcept { return vk_imageview_; }

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapChain_;

  // components
  VkImage vk_image_{VK_NULL_HANDLE};
  VkDeviceMemory vk_memory_{VK_NULL_HANDLE};
  VkImageView vk_imageview_{VK_NULL_HANDLE};

  VkFormat findDepthFormat() {
    return findSupportedFormat({VK_FORMAT_D32_SFLOAT,
                                VK_FORMAT_D32_SFLOAT_S8_UINT,
                                VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(device_.physicalDevice(), format,
                                          &props);
      if (tiling == VK_IMAGE_TILING_LINEAR &&
          (props.linearTilingFeatures & features) == features) {
        return format;
      } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                 (props.optimalTilingFeatures & features) == features) {
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format!");
  }
};

} // namespace vkr::resources
