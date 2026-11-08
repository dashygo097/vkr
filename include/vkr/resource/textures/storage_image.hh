#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/texture.hh"

namespace vkr::res {

struct StorageImageDesc {
  ImageDesc image{};
  VkImageLayout layout{VK_IMAGE_LAYOUT_GENERAL};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return image.isValid() && (image.usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0 &&
           image.type == VK_IMAGE_TYPE_2D && image.depth == 1 &&
           image.arrayLayers == 1 && image.mipLevels == 1 &&
           image.samples == VK_SAMPLE_COUNT_1_BIT &&
           image.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT &&
           image.defaultViewType == VK_IMAGE_VIEW_TYPE_2D &&
           image.layout == VK_IMAGE_LAYOUT_UNDEFINED;
  }

  [[nodiscard]] static auto storage2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> StorageImageDesc {
    StorageImageDesc desc{};
    desc.image = ImageDesc::storage2D(width, height, format);
    return desc;
  }
};

class StorageImage final : public Texture {
public:
  explicit StorageImage(const core::Device &device,
                        const core::CommandPool &commandPool);
  ~StorageImage() override = default;

  void update(const StorageImageDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const StorageImageDesc & {
    return desc_;
  }

private:
  // components
  const core::CommandPool &command_pool_;

  // components
  StorageImageDesc desc_{};
};

} // namespace vkr::res
