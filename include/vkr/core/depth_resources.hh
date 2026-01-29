#pragma once

#include "./device.hh"
#include "./swapchain.hh"

namespace vkr::core {
class DepthResources {
public:
  explicit DepthResources(const Device &device, const Swapchain &swapChain);
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
  const Device &device;
  const Swapchain &swapChain;

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
      vkGetPhysicalDeviceFormatProperties(device.physicalDevice(), format,
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

} // namespace vkr::core
