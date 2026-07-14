#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/gpu/image.hh"
#include "vkr/resource/gpu/image_view.hh"
#include "vkr/resource/gpu/sampler.hh"

namespace vkr::resource {

struct TextureDesc {
  ImageDesc image{};
  ImageViewDesc view{};
  SamplerDesc sampler{SamplerDesc::linearRepeat()};

  bool useDefaultView{true};
  bool createSampler{true};

  [[nodiscard]] static auto textureFile(
      const std::string &path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
      SamplerDesc sampler = SamplerDesc::linearRepeat()) -> TextureDesc {
    TextureDesc desc{};
    desc.image = ImageDesc::textureFile(path, format);
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  cubemapFiles(const std::array<std::string, 6> &faces,
               VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
               SamplerDesc sampler = SamplerDesc::linearClampToEdge())
      -> TextureDesc {
    TextureDesc desc{};
    desc.image = ImageDesc::cubemapFiles(faces, format);
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  sampled2D(uint32_t width, uint32_t height, VkFormat format,
            SamplerDesc sampler = SamplerDesc::linearClampToEdge())
      -> TextureDesc {
    TextureDesc desc{};
    desc.image = ImageDesc::sampled2D(width, height, format);
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto storage2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> TextureDesc {
    TextureDesc desc{};
    desc.image = ImageDesc::empty2D(
        width, height, format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    desc.sampler = SamplerDesc::linearClampToEdge();
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  colorAttachment(uint32_t width, uint32_t height, VkFormat format,
                  SamplerDesc sampler = SamplerDesc::linearClampToEdge())
      -> TextureDesc {
    TextureDesc desc{};
    desc.image = ImageDesc::colorAttachment(width, height, format);
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto depthAttachment(uint32_t width, uint32_t height,
                                            VkFormat format) -> TextureDesc {
    TextureDesc desc{};
    desc.image = ImageDesc::depthAttachment(width, height, format);
    desc.sampler = SamplerDesc::shadowCompare();
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] static auto
  fromImageDesc(ImageDesc image,
                SamplerDesc sampler = SamplerDesc::linearRepeat())
      -> TextureDesc {
    TextureDesc desc{};
    desc.image = std::move(image);
    desc.sampler = sampler;
    desc.useDefaultView = true;
    desc.createSampler = true;
    return desc;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    if (image.format == VK_FORMAT_UNDEFINED) {
      return false;
    }

    if (!image.hasFile() && (image.width == 0 || image.height == 0)) {
      if (image.hasCubemapFiles()) {
        return true;
      }

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
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  TextureDesc desc_{};
  std::unique_ptr<Image> image_;
  std::unique_ptr<ImageView> image_view_;
  std::unique_ptr<Sampler> sampler_;
};

} // namespace vkr::resource
