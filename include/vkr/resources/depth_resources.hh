#pragma once

#include "../core/device.hh"
#include "../core/swapchain.hh"
#include "./textures/image.hh"
#include "./textures/imageview.hh"

namespace vkr::resource {

class DepthResources {
public:
  explicit DepthResources(const core::Device &device,
                          const core::Swapchain &swapchain,
                          const core::CommandPool &commandPool);

  ~DepthResources() = default;

  DepthResources(const DepthResources &) = delete;
  auto operator=(const DepthResources &) -> DepthResources & = delete;

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
