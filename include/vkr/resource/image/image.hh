#pragma once

#include "vkr/core/device.hh"
#include <vulkan/vulkan.h>

namespace vkr::resource {

struct ImageDesc {
  uint32_t width{0};
  uint32_t height{0};
  uint32_t depth{1};
  uint32_t mipLevels{1};
  uint32_t arrayLayers{1};

  VkImageType type{VK_IMAGE_TYPE_2D};
  VkImageCreateFlags flags{0};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageTiling tiling{VK_IMAGE_TILING_OPTIMAL};
  VkImageUsageFlags usage{0};
  VkMemoryPropertyFlags memoryProperties{VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
  VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
  VkSharingMode sharingMode{VK_SHARING_MODE_EXCLUSIVE};

  VkImageAspectFlags aspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
  VkImageViewType defaultViewType{VK_IMAGE_VIEW_TYPE_2D};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return width != 0 && height != 0 && depth != 0 && mipLevels != 0 &&
           arrayLayers != 0 && format != VK_FORMAT_UNDEFINED && usage != 0;
  }

  [[nodiscard]] static auto empty2D(
      uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
      VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) -> ImageDesc {
    ImageDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.format = format;
    desc.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.usage = usage;
    desc.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.aspectMask = aspectMask;
    return desc;
  }

  [[nodiscard]] static auto sampled2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> ImageDesc {
    return empty2D(width, height, format,
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                   VK_IMAGE_ASPECT_COLOR_BIT);
  }

  [[nodiscard]] static auto colorAttachment(uint32_t width, uint32_t height,
                                            VkFormat format) -> ImageDesc {
    return empty2D(width, height, format,
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                       VK_IMAGE_USAGE_SAMPLED_BIT,
                   VK_IMAGE_ASPECT_COLOR_BIT);
  }

  [[nodiscard]] static auto depthAttachment(uint32_t width, uint32_t height,
                                            VkFormat format) -> ImageDesc {
    return empty2D(width, height, format,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                   VK_IMAGE_ASPECT_DEPTH_BIT);
  }

  [[nodiscard]] static auto storage2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> ImageDesc {
    return empty2D(width, height, format,
                   VK_IMAGE_USAGE_STORAGE_BIT |
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                   VK_IMAGE_ASPECT_COLOR_BIT);
  }
};

class Image {
public:
  explicit Image(const core::Device &device);
  Image(const core::Device &device, const ImageDesc &desc);
  ~Image();

  Image(const Image &) = delete;
  auto operator=(const Image &) -> Image & = delete;

  void destroy();
  void update(const ImageDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const ImageDesc & {
    return desc_;
  }
  [[nodiscard]] auto width() const noexcept -> uint32_t { return desc_.width; }
  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return desc_.height;
  }
  [[nodiscard]] auto depth() const noexcept -> uint32_t { return desc_.depth; }
  [[nodiscard]] auto layout() const noexcept -> VkImageLayout {
    return layout_;
  }
  void setLayout(VkImageLayout layout) noexcept { layout_ = layout; }

  [[nodiscard]] auto image() const noexcept -> VkImage { return vk_image_; }
  [[nodiscard]] auto memory() const noexcept -> VkDeviceMemory {
    return vk_memory_;
  }
  [[nodiscard]] auto isValid() const noexcept -> bool {
    return vk_image_ != VK_NULL_HANDLE && vk_memory_ != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  ImageDesc desc_{};
  VkImageLayout layout_{VK_IMAGE_LAYOUT_UNDEFINED};
  VkImage vk_image_{VK_NULL_HANDLE};
  VkDeviceMemory vk_memory_{VK_NULL_HANDLE};

  // helpers
  void create();
};

} // namespace vkr::resource
