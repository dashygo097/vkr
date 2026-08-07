#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/image/image.hh"
#include "vkr/resource/image/image_view.hh"
#include "vkr/resource/image/sampler.hh"
#include <utility>

namespace vkr::exec {

struct ColorAttachmentDesc {
  uint32_t width{};
  uint32_t height{};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageUsageFlags usage{VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT};
  VkImageLayout finalLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  bool createSampler{false};
  resource::SamplerDesc sampler{resource::SamplerDesc::linearClampToEdge()};

  auto extent(uint32_t attachmentWidth, uint32_t attachmentHeight) noexcept
      -> ColorAttachmentDesc & {
    width = attachmentWidth;
    height = attachmentHeight;
    return *this;
  }

  auto imageFormat(VkFormat attachmentFormat) noexcept
      -> ColorAttachmentDesc & {
    format = attachmentFormat;
    return *this;
  }

  auto usageFlags(VkImageUsageFlags flags) noexcept -> ColorAttachmentDesc & {
    usage = flags;
    return *this;
  }

  auto addUsage(VkImageUsageFlags flags) noexcept -> ColorAttachmentDesc & {
    usage |= flags;
    return *this;
  }

  auto sampled(bool enabled = true) noexcept -> ColorAttachmentDesc & {
    if (enabled) {
      usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
      createSampler = true;
      if (finalLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      return *this;
    }

    usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    createSampler = false;
    return *this;
  }

  auto storage(bool enabled = true) noexcept -> ColorAttachmentDesc & {
    if (enabled) {
      usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    } else {
      usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
    }
    return *this;
  }

  auto finalImageLayout(VkImageLayout layout) noexcept
      -> ColorAttachmentDesc & {
    finalLayout = layout;
    return *this;
  }

  auto withSampler(resource::SamplerDesc desc) -> ColorAttachmentDesc & {
    sampler = std::move(desc);
    createSampler = true;
    return *this;
  }

  auto samplerEnabled(bool enabled = true) noexcept -> ColorAttachmentDesc & {
    createSampler = enabled;
    return *this;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return width != 0 && height != 0 && format != VK_FORMAT_UNDEFINED;
  }

  [[nodiscard]] static auto attachment(uint32_t width, uint32_t height,
                                       VkFormat format) -> ColorAttachmentDesc {
    ColorAttachmentDesc desc{};
    return desc.extent(width, height)
        .imageFormat(format)
        .usageFlags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  }

  [[nodiscard]] static auto sampled2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> ColorAttachmentDesc {
    ColorAttachmentDesc desc{};
    return desc.extent(width, height)
        .imageFormat(format)
        .usageFlags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        .sampled();
  }
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

  [[nodiscard]] auto hasSampler() const noexcept -> bool {
    return sampler_ && sampler_->sampler() != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  ColorAttachmentDesc desc_{};
  std::unique_ptr<resource::Image> image_;
  std::unique_ptr<resource::ImageView> image_view_;
  std::unique_ptr<resource::Sampler> sampler_;
};

} // namespace vkr::exec
