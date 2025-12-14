#pragma once

#include "../../ctx.hh"

namespace vkr {
class DepthResources {
public:
  DepthResources(VkPhysicalDevice physicalDevice, VkExtent2D swapChainExtent);
  DepthResources(const VulkanContext &ctx);
  ~DepthResources();

  DepthResources(const DepthResources &) = delete;
  DepthResources &operator=(const DepthResources &) = delete;

  [[nodiscard]] VkImage image() const noexcept { return _image; }
  [[nodiscard]] VkDeviceMemory imageMemory() const noexcept {
    return _imageMemory;
  }
  [[nodiscard]] VkImageView imageView() const noexcept { return _imageView; }

private:
  // dependencies
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkExtent2D swapchainExtent{};

  // components
  VkImage _image{VK_NULL_HANDLE};
  VkDeviceMemory _imageMemory{VK_NULL_HANDLE};
  VkImageView _imageView{VK_NULL_HANDLE};

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
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
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

} // namespace vkr
