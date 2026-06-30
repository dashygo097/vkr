#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/resource/gpu/image.hh"
#include "vkr/resource/gpu/image_view.hh"

namespace vkr::resource {

class DepthTarget {
public:
  explicit DepthTarget(const core::Device &device,
                       const core::Swapchain &swapchain,
                       const core::CommandPool &commandPool);

  ~DepthTarget() = default;

  DepthTarget(const DepthTarget &) = delete;
  auto operator=(const DepthTarget &) -> DepthTarget & = delete;

  [[nodiscard]] auto image() const noexcept -> VkImage {
    return image_->image();
  }
  [[nodiscard]] auto imageView() const noexcept -> VkImageView {
    return imageview_->imageView();
  }

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapchain_;
  const core::CommandPool &command_pool_;

  // components
  std::unique_ptr<Image> image_;
  std::unique_ptr<ImageView> imageview_;
};

// helpers
auto findSupportedFormat(VkPhysicalDevice physicalDevice,
                         const std::vector<VkFormat> &candidates,
                         VkImageTiling tiling, VkFormatFeatureFlags features)
    -> VkFormat;
auto findDepthFormat(VkPhysicalDevice physicalDevice) -> VkFormat;

} // namespace vkr::resource
