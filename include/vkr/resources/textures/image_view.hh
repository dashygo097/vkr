#pragma once

#include "../../core/device.hh"
#include "./image.hh"

namespace vkr {
class ImageView {
public:
  explicit ImageView(const Device &device);
  ~ImageView();

  ImageView(const ImageView &) = delete;
  ImageView &operator=(const ImageView &) = delete;

  void create(const Image &image, VkFormat format);
  void destroy();
  void update(const Image &image, VkFormat format);

  [[nodiscard]] VkFormat format() const { return _format; }
  [[nodiscard]] VkImageView imageView() const { return _image_view; }

private:
  // dependencies
  const Device &device;

  // components
  const Image &_image;
  VkFormat _format;
  VkImageView _image_view;
};
} // namespace vkr
