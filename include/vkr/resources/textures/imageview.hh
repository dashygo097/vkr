#pragma once

#include "../../core/device.hh"
#include "./image.hh"

namespace vkr::resource {
class ImageView {
public:
  explicit ImageView(const core::Device &device);
  ~ImageView();

  ImageView(const ImageView &) = delete;
  ImageView &operator=(const ImageView &) = delete;

  void create(const Image &image, VkFormat format);
  void destroy();
  void update(const Image &image, VkFormat format) {
    destroy();
    create(image, format);
  }

  [[nodiscard]] VkImageView imageView() const noexcept { return vk_imageview_; }

private:
  // dependencies
  const core::Device &device_;

  // components
  VkImageView vk_imageview_{VK_NULL_HANDLE};
};
} // namespace vkr::resource
