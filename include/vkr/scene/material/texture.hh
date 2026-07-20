#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/gpu/image.hh"
#include "vkr/resource/gpu/image_view.hh"
#include "vkr/resource/gpu/sampler.hh"
#include <memory>
#include <string>
#include <utility>

namespace vkr::scene {

struct TextureDesc {
  resource::ImageDesc image{};
  resource::ImageViewDesc view{};
  resource::SamplerDesc sampler{resource::SamplerDesc::linearRepeat()};

  std::string filePath{};
  VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};

  bool useDefaultView{true};
  bool createSampler{true};
  bool forceRgba{true};

  [[nodiscard]] static auto textureFile(
      const std::string &path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
      resource::SamplerDesc sampler = resource::SamplerDesc::linearRepeat())
      -> TextureDesc {
    TextureDesc desc{};
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
            resource::SamplerDesc sampler =
                resource::SamplerDesc::linearClampToEdge())
      -> TextureDesc {
    TextureDesc desc{};
    desc.image = resource::ImageDesc::sampled2D(width, height, format);
    desc.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto storage2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> TextureDesc {
    TextureDesc desc{};
    desc.image = resource::ImageDesc::storage2D(width, height, format);
    desc.image.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.layout = VK_IMAGE_LAYOUT_GENERAL;
    desc.sampler = resource::SamplerDesc::linearClampToEdge();
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  colorAttachment(uint32_t width, uint32_t height, VkFormat format,
                  resource::SamplerDesc sampler =
                      resource::SamplerDesc::linearClampToEdge())
      -> TextureDesc {
    TextureDesc desc{};
    desc.image = resource::ImageDesc::colorAttachment(width, height, format);
    desc.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto depthAttachment(uint32_t width, uint32_t height,
                                            VkFormat format) -> TextureDesc {
    TextureDesc desc{};
    desc.image = resource::ImageDesc::depthAttachment(width, height, format);
    desc.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    desc.sampler = resource::SamplerDesc::shadowCompare();
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  fromImageDesc(resource::ImageDesc image,
                resource::SamplerDesc sampler =
                    resource::SamplerDesc::linearRepeat())
      -> TextureDesc {
    TextureDesc desc{};
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

class Texture {
public:
  Texture(const core::Device &device, const core::CommandPool &commandPool);
  ~Texture();

  Texture(const Texture &) = delete;
  auto operator=(const Texture &) -> Texture & = delete;

  void create();
  void destroy();
  void update(const TextureDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const TextureDesc & {
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

  [[nodiscard]] auto image() const -> VkImage { return image_->image(); }
  [[nodiscard]] auto imageView() const -> VkImageView {
    return image_view_->imageView();
  }
  [[nodiscard]] auto sampler() const -> VkSampler {
    if (!hasSampler()) {
      return VK_NULL_HANDLE;
    }

    return sampler_->sampler();
  }

  [[nodiscard]] auto descriptorInfo() const noexcept -> VkDescriptorImageInfo {
    VkDescriptorImageInfo info{};
    info.sampler = hasSampler() ? sampler_->sampler() : VK_NULL_HANDLE;
    info.imageView = hasImageView() ? image_view_->imageView() : VK_NULL_HANDLE;
    info.imageLayout = layout();
    return info;
  }

  [[nodiscard]] auto hasImage() const noexcept -> bool {
    return image_ != nullptr && image_->image() != VK_NULL_HANDLE;
  }

  [[nodiscard]] auto hasImageView() const noexcept -> bool {
    return image_view_ != nullptr && image_view_->imageView() != VK_NULL_HANDLE;
  }

  [[nodiscard]] auto hasSampler() const noexcept -> bool {
    return sampler_ != nullptr && sampler_->sampler() != VK_NULL_HANDLE;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return hasImage() && hasImageView();
  }

private:
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  TextureDesc desc_{};
  std::unique_ptr<resource::Image> image_;
  std::unique_ptr<resource::ImageView> image_view_;
  std::unique_ptr<resource::Sampler> sampler_;

  void createFromFile();
  void createEmpty();
  void createViewAndSampler();
};

} // namespace vkr::scene
