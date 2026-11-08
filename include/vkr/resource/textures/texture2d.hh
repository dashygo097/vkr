#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/sampler.hh"
#include "vkr/resource/texture.hh"
#include <optional>
#include <string>
#include <utility>

namespace vkr::res {

struct Texture2DDesc {
  ImageDesc image{};
  ImageViewDesc view{};
  SamplerDesc sampler{SamplerDesc::linearRepeat()};

  std::string filePath{};
  VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};

  bool useDefaultView{true};
  bool createSampler{true};
  bool forceRgba{true};

  [[nodiscard]] static auto textureFile(
      const std::string &path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
      SamplerDesc sampler = SamplerDesc::linearRepeat()) -> Texture2DDesc {
    Texture2DDesc desc{};
    desc.filePath = path;
    desc.image.format = format;
    desc.image.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.image.usage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.image.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.image.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    desc.image.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    desc.forceRgba = true;
    return desc;
  }

  [[nodiscard]] static auto
  sampled2D(uint32_t width, uint32_t height, VkFormat format,
            SamplerDesc sampler = SamplerDesc::linearClampToEdge())
      -> Texture2DDesc {
    Texture2DDesc desc{};
    desc.image = ImageDesc::sampled2D(width, height, format);
    desc.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto storage2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> Texture2DDesc {
    Texture2DDesc desc{};
    desc.image = ImageDesc::storage2D(width, height, format);
    desc.image.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.layout = VK_IMAGE_LAYOUT_GENERAL;
    desc.sampler = SamplerDesc::linearClampToEdge();
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  colorAttachment(uint32_t width, uint32_t height, VkFormat format,
                  SamplerDesc sampler = SamplerDesc::linearClampToEdge())
      -> Texture2DDesc {
    Texture2DDesc desc{};
    desc.image = ImageDesc::colorAttachment(width, height, format);
    desc.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto depthAttachment(uint32_t width, uint32_t height,
                                            VkFormat format) -> Texture2DDesc {
    Texture2DDesc desc{};
    desc.image = ImageDesc::depthAttachment(width, height, format);
    desc.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    desc.sampler = SamplerDesc::shadowCompare();
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  fromImageDesc(ImageDesc image,
                SamplerDesc sampler = SamplerDesc::linearRepeat())
      -> Texture2DDesc {
    Texture2DDesc desc{};
    desc.image = std::move(image);
    desc.layout = desc.image.layout;
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    if (image.format == VK_FORMAT_UNDEFINED) {
      return false;
    }

    if (filePath.empty() && (image.width == 0 || image.height == 0)) {
      return false;
    }

    return true;
  }
};

class Texture2D final : public Texture {
public:
  explicit Texture2D(const core::Device &device,
                     const core::CommandPool &commandPool);
  ~Texture2D() override = default;

  void create();
  void destroy() noexcept override;
  void update(const Texture2DDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const Texture2DDesc & {
    return desc_;
  }
  [[nodiscard]] auto sampler() const noexcept -> VkSampler {
    return sampler_ ? sampler_->sampler() : VK_NULL_HANDLE;
  }
  [[nodiscard]] auto hasSampler() const noexcept -> bool {
    return sampler() != VK_NULL_HANDLE;
  }
  [[nodiscard]] auto descriptorInfo() const noexcept -> VkDescriptorImageInfo {
    return {sampler(), imageView(), layout()};
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  Texture2DDesc desc_{};
  std::optional<Sampler> sampler_{};
};

} // namespace vkr::res
