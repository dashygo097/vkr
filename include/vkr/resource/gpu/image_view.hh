#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/gpu/image.hh"
#include <vulkan/vulkan.h>

namespace vkr::resource {

struct ImageViewDesc {
  VkImage image{VK_NULL_HANDLE};

  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageViewType viewType{VK_IMAGE_VIEW_TYPE_2D};
  VkImageAspectFlags aspectMask{VK_IMAGE_ASPECT_COLOR_BIT};

  uint32_t baseMipLevel{0};
  uint32_t levelCount{1};
  uint32_t baseArrayLayer{0};
  uint32_t layerCount{1};

  VkComponentMapping components{
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
  };

  [[nodiscard]] static auto fromImage(const Image &image) -> ImageViewDesc {
    ImageViewDesc desc{};
    desc.image = image.image();
    desc.format = image.desc().format;
    desc.viewType = image.desc().defaultViewType;
    desc.aspectMask = image.desc().aspectMask;
    desc.baseMipLevel = 0;
    desc.levelCount = image.desc().mipLevels;
    desc.baseArrayLayer = 0;
    desc.layerCount = image.desc().arrayLayers;
    return desc;
  }

  [[nodiscard]] static auto cubemap(VkImage image, VkFormat format)
      -> ImageViewDesc {
    ImageViewDesc desc{};
    desc.image = image;
    desc.format = format;
    desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    desc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    desc.baseMipLevel = 0;
    desc.levelCount = 1;
    desc.baseArrayLayer = 0;
    desc.layerCount = 6;
    return desc;
  }

  [[nodiscard]] static auto color2D(VkImage image, VkFormat format)
      -> ImageViewDesc {
    ImageViewDesc desc{};
    desc.image = image;
    desc.format = format;
    desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    desc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    return desc;
  }

  [[nodiscard]] static auto depth2D(VkImage image, VkFormat format)
      -> ImageViewDesc {
    ImageViewDesc desc{};
    desc.image = image;
    desc.format = format;
    desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    desc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    return desc;
  }

  [[nodiscard]] static auto depthStencil2D(VkImage image, VkFormat format)
      -> ImageViewDesc {
    ImageViewDesc desc{};
    desc.image = image;
    desc.format = format;
    desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    desc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    return desc;
  }
};

class ImageView {
public:
  explicit ImageView(const core::Device &device);
  ~ImageView();

  ImageView(const ImageView &) = delete;
  auto operator=(const ImageView &) -> ImageView & = delete;

  void create();
  void destroy();
  void update(const ImageViewDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const ImageViewDesc & {
    return desc_;
  }

  [[nodiscard]] auto imageView() const noexcept -> VkImageView {
    return vk_imageview_;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  ImageViewDesc desc_{};
  VkImageView vk_imageview_{VK_NULL_HANDLE};
};

} // namespace vkr::resource
