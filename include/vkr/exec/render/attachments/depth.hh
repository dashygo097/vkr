#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/image/image.hh"
#include "vkr/resource/image/image_view.hh"
#include "vkr/resource/image/sampler.hh"
#include <utility>

namespace vkr::exec {

struct DepthAttachmentDesc {
  uint32_t width{};
  uint32_t height{};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageUsageFlags usage{VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT};
  VkImageLayout finalLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  bool createSampler{false};
  resource::SamplerDesc sampler{resource::SamplerDesc::nearestClampToEdge()};

  auto extent(uint32_t attachmentWidth, uint32_t attachmentHeight) noexcept
      -> DepthAttachmentDesc & {
    width = attachmentWidth;
    height = attachmentHeight;
    return *this;
  }

  auto imageFormat(VkFormat attachmentFormat) noexcept
      -> DepthAttachmentDesc & {
    format = attachmentFormat;
    return *this;
  }

  auto usageFlags(VkImageUsageFlags flags) noexcept -> DepthAttachmentDesc & {
    usage = flags;
    return *this;
  }

  auto addUsage(VkImageUsageFlags flags) noexcept -> DepthAttachmentDesc & {
    usage |= flags;
    return *this;
  }

  auto sampled(bool enabled = true) noexcept -> DepthAttachmentDesc & {
    if (enabled) {
      usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
      finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      createSampler = true;
      return *this;
    }

    usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createSampler = false;
    return *this;
  }

  auto finalImageLayout(VkImageLayout layout) noexcept
      -> DepthAttachmentDesc & {
    finalLayout = layout;
    return *this;
  }

  auto withSampler(resource::SamplerDesc desc) -> DepthAttachmentDesc & {
    sampler = std::move(desc);
    createSampler = true;
    return *this;
  }

  auto samplerEnabled(bool enabled = true) noexcept -> DepthAttachmentDesc & {
    createSampler = enabled;
    return *this;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return width != 0 && height != 0 && format != VK_FORMAT_UNDEFINED;
  }

  [[nodiscard]] static auto attachment(uint32_t width, uint32_t height,
                                       VkFormat format) -> DepthAttachmentDesc {
    DepthAttachmentDesc desc{};
    return desc.extent(width, height)
        .imageFormat(format)
        .usageFlags(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  [[nodiscard]] static auto sampled2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> DepthAttachmentDesc {
    DepthAttachmentDesc desc{};
    return desc.extent(width, height)
        .imageFormat(format)
        .usageFlags(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        .sampled();
  }

  [[nodiscard]] static auto shadowMap(uint32_t width, uint32_t height,
                                      VkFormat format = VK_FORMAT_D32_SFLOAT)
      -> DepthAttachmentDesc {
    return sampled2D(width, height, format);
  }
};

class DepthAttachment {
public:
  explicit DepthAttachment(const core::Device &device,
                           const core::CommandPool &commandPool);

  ~DepthAttachment();

  DepthAttachment(const DepthAttachment &) = delete;
  auto operator=(const DepthAttachment &) -> DepthAttachment & = delete;

  [[nodiscard]] auto desc() const noexcept -> const DepthAttachmentDesc & {
    return desc_;
  }

  void create();
  void destory();
  void update(const DepthAttachmentDesc &desc);

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
  DepthAttachmentDesc desc_{};
  std::unique_ptr<resource::Image> image_;
  std::unique_ptr<resource::ImageView> image_view_;
  std::unique_ptr<resource::Sampler> sampler_;
};

} // namespace vkr::exec
