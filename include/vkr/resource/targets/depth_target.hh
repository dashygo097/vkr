#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/gpu/image.hh"
#include "vkr/resource/gpu/image_view.hh"

namespace vkr::resource {

struct DepthTargetDesc {
  uint32_t width{};
  uint32_t height{};
  VkFormat format{VK_FORMAT_UNDEFINED};
};

class DepthTarget {
public:
  explicit DepthTarget(const core::Device &device,
                       const core::CommandPool &commandPool);

  ~DepthTarget();

  DepthTarget(const DepthTarget &) = delete;
  auto operator=(const DepthTarget &) -> DepthTarget & = delete;

  [[nodiscard]] auto desc() const noexcept -> const DepthTargetDesc & {
    return desc_;
  }

  void create();
  void destory();
  void update(const DepthTargetDesc &desc);

  [[nodiscard]] auto image() const noexcept -> VkImage {
    return image_->image();
  }
  [[nodiscard]] auto imageView() const noexcept -> VkImageView {
    return image_view_->imageView();
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  DepthTargetDesc desc_{};
  ImageDesc image_desc_{};
  std::unique_ptr<Image> image_;
  std::unique_ptr<ImageView> image_view_;
};

// helpers
auto findSupportedFormat(VkPhysicalDevice physicalDevice,
                         const std::vector<VkFormat> &candidates,
                         VkImageTiling tiling, VkFormatFeatureFlags features)
    -> VkFormat;
auto findDepthFormat(VkPhysicalDevice physicalDevice) -> VkFormat;

} // namespace vkr::resource
