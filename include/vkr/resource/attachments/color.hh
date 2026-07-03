#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/gpu/image.hh"
#include "vkr/resource/gpu/image_view.hh"
#include "vkr/resource/gpu/sampler.hh"

namespace vkr::resource {

struct ColorAttachmentDesc {
  uint32_t width{};
  uint32_t height{};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageUsageFlags usage{VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT};
  VkImageLayout finalLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  bool createSampler{false};
  SamplerDesc sampler{SamplerDesc::linearClampToEdge()};
};

class ColorAttachment {
public:
  explicit ColorAttachment(const core::Device &device,
                           const core::CommandPool &commandPool);

  ~ColorAttachment();

  ColorAttachment(const ColorAttachment &) = delete;
  auto operator=(const ColorAttachment &) -> ColorAttachment & = delete;

  [[nodiscard]] auto desc() const noexcept -> const ColorAttachmentDesc & {
    return desc_;
  }

  void create();
  void destory();
  void update(const ColorAttachmentDesc &desc);

  [[nodiscard]] auto image() const noexcept -> VkImage {
    return image_->image();
  }

  [[nodiscard]] auto imageView() const noexcept -> VkImageView {
    return image_view_->imageView();
  }

  [[nodiscard]] auto sampler() const noexcept -> VkSampler {
    return sampler_ ? sampler_->sampler() : VK_NULL_HANDLE;
  }

  [[nodiscard]] auto samplerRef() const noexcept -> const Sampler * {
    return sampler_.get();
  }

  [[nodiscard]] auto hasSampler() const noexcept -> bool {
    return sampler_ && sampler_->sampler() != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  ColorAttachmentDesc desc_{};
  std::unique_ptr<Image> image_;
  std::unique_ptr<ImageView> image_view_;
  std::unique_ptr<Sampler> sampler_;
};

} // namespace vkr::resource
