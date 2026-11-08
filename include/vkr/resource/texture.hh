#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/resource/image.hh"
#include "vkr/resource/image_view.hh"
#include <optional>

namespace vkr::res {

class Buffer;

class Texture {
public:
  virtual ~Texture() = default;

  Texture(const Texture &) = delete;
  auto operator=(const Texture &) -> Texture & = delete;

  virtual void destroy() noexcept;

  [[nodiscard]] auto width() const noexcept -> uint32_t {
    return hasImage() ? image_.width() : 0;
  }
  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return hasImage() ? image_.height() : 0;
  }
  [[nodiscard]] auto format() const noexcept -> VkFormat {
    return image_.desc().format;
  }
  [[nodiscard]] auto usage() const noexcept -> VkImageUsageFlags {
    return image_.desc().usage;
  }
  [[nodiscard]] auto layout() const noexcept -> VkImageLayout {
    return image_.layout();
  }
  [[nodiscard]] auto image() const noexcept -> VkImage {
    return image_.image();
  }
  [[nodiscard]] auto imageView() const noexcept -> VkImageView {
    return image_view_.imageView();
  }
  [[nodiscard]] auto hasImage() const noexcept -> bool {
    return image_.isValid();
  }
  [[nodiscard]] auto hasImageView() const noexcept -> bool {
    return imageView() != VK_NULL_HANDLE;
  }
  [[nodiscard]] auto valid() const noexcept -> bool {
    return hasImage() && hasImageView();
  }
  [[nodiscard]] auto descriptorInfo() const noexcept -> VkDescriptorImageInfo {
    return {VK_NULL_HANDLE, imageView(), layout()};
  }

protected:
  explicit Texture(const core::Device &device);

  void create(const ImageDesc &image,
              const std::optional<ImageViewDesc> &view = std::nullopt);
  void transition(const core::CommandPool &commandPool, VkImageLayout layout);
  void upload(const core::CommandPool &commandPool, const Buffer &buffer,
              uint32_t channels, VkImageLayout layout);

private:
  // components
  Image image_;
  ImageView image_view_;

  // helpers
  void recordBarrier(const core::CommandBuffer &commands, VkImageLayout source,
                     VkImageLayout destination) const;
  void validateQueue(const core::CommandPool &commandPool,
                     VkImageLayout layout) const;
};

} // namespace vkr::res
