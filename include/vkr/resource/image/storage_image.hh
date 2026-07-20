#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/image/image.hh"
#include "vkr/resource/image/image_view.hh"
#include <memory>

namespace vkr::resource {

struct StorageImageDesc {
  uint32_t width{0};
  uint32_t height{0};
  VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
  VkImageUsageFlags usage{VK_IMAGE_USAGE_STORAGE_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT};
  VkImageAspectFlags aspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
  VkImageLayout layout{VK_IMAGE_LAYOUT_GENERAL};
  VkImageViewType viewType{VK_IMAGE_VIEW_TYPE_2D};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return width != 0 && height != 0 && format != VK_FORMAT_UNDEFINED &&
           (usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0;
  }

  [[nodiscard]] static auto storage2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> StorageImageDesc {
    StorageImageDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.format = format;
    return desc;
  }
};

class StorageImage {
public:
  StorageImage(const core::Device &device,
               const core::CommandPool &commandPool);
  ~StorageImage();

  StorageImage(const StorageImage &) = delete;
  auto operator=(const StorageImage &) -> StorageImage & = delete;

  void update(const StorageImageDesc &desc);
  void destroy();

  [[nodiscard]] auto desc() const noexcept -> const StorageImageDesc & {
    return desc_;
  }

  [[nodiscard]] auto width() const noexcept -> uint32_t {
    return image_ ? image_->width() : 0;
  }

  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return image_ ? image_->height() : 0;
  }

  [[nodiscard]] auto layout() const noexcept -> VkImageLayout {
    return image_ ? image_->layout() : VK_IMAGE_LAYOUT_UNDEFINED;
  }

  [[nodiscard]] auto image() const noexcept -> VkImage {
    return image_ ? image_->image() : VK_NULL_HANDLE;
  }

  [[nodiscard]] auto imageView() const noexcept -> VkImageView {
    return image_view_ ? image_view_->imageView() : VK_NULL_HANDLE;
  }

  [[nodiscard]] auto descriptorInfo() const noexcept -> VkDescriptorImageInfo;

  [[nodiscard]] auto valid() const noexcept -> bool {
    return image() != VK_NULL_HANDLE && imageView() != VK_NULL_HANDLE;
  }

private:
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  StorageImageDesc desc_{};
  std::unique_ptr<Image> image_;
  std::unique_ptr<ImageView> image_view_;

  void create();
};

} // namespace vkr::resource
