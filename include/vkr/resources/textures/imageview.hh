#pragma once

#include "../../core/device.hh"
#include "./image.hh"

namespace vkr::resource {
class ImageView {
public:
  explicit ImageView(const core::Device &device);
  ~ImageView();

  ImageView(const ImageView &) = delete;
  auto operator=(const ImageView &) -> ImageView & = delete;

  void create(const Image &image, VkFormat format,
              VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
  void destroy();
  void update(const Image &image, VkFormat format,
              VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
    destroy();
    create(image, format);
  }

  [[nodiscard]] auto imageView() const noexcept -> VkImageView { return vk_imageview_; }

private:
  // dependencies
  const core::Device &device_;

  // components
  VkImageView vk_imageview_{VK_NULL_HANDLE};
};
} // namespace vkr::resource
