#pragma once

#include "../../core/device.hh"
#include "./image.hh"

namespace vkr::resource {
class ImageView {
public:
  explicit ImageView(const core::Device &device, const Image &image);
  ~ImageView();

  ImageView(const ImageView &) = delete;
  ImageView &operator=(const ImageView &) = delete;

  void create(const Image &image, VkFormat format);
  void destroy();
  void update(const Image &image, VkFormat format);

  [[nodiscard]] VkFormat format() const { return vk_format_; }
  [[nodiscard]] VkImageView imageView() const { return vk_imageview_; }

private:
  // dependencies
  const core::Device &device_;
  const Image &image_;

  // components
  VkFormat vk_format_;
  VkImageView vk_imageview_;
};
} // namespace vkr::resource
