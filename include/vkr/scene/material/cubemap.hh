#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/gpu/image.hh"
#include "vkr/resource/gpu/image_view.hh"
#include "vkr/resource/gpu/sampler.hh"
#include <array>
#include <memory>
#include <string>

namespace vkr::scene {

struct CubemapDesc {
  std::array<std::string, 6> facePaths{};
  VkFormat format{VK_FORMAT_R8G8B8A8_SRGB};
  VkImageLayout layout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  resource::SamplerDesc sampler{
      resource::SamplerDesc::linearClampToEdge()};
  bool forceRgba{true};

  [[nodiscard]] static auto
  files(const std::array<std::string, 6> &faces,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        resource::SamplerDesc sampler =
            resource::SamplerDesc::linearClampToEdge()) -> CubemapDesc {
    CubemapDesc desc{};
    desc.facePaths = faces;
    desc.format = format;
    desc.sampler = sampler;
    return desc;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    if (format == VK_FORMAT_UNDEFINED) {
      return false;
    }

    for (const auto &path : facePaths) {
      if (path.empty()) {
        return false;
      }
    }

    return true;
  }
};

class Cubemap {
public:
  Cubemap(const core::Device &device, const core::CommandPool &commandPool);
  ~Cubemap();

  Cubemap(const Cubemap &) = delete;
  auto operator=(const Cubemap &) -> Cubemap & = delete;

  void destroy();
  void update(const CubemapDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const CubemapDesc & {
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

  [[nodiscard]] auto sampler() const noexcept -> VkSampler {
    return sampler_ ? sampler_->sampler() : VK_NULL_HANDLE;
  }

  [[nodiscard]] auto descriptorInfo() const noexcept -> VkDescriptorImageInfo {
    VkDescriptorImageInfo info{};
    info.sampler = sampler();
    info.imageView = imageView();
    info.imageLayout = layout();
    return info;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return image() != VK_NULL_HANDLE && imageView() != VK_NULL_HANDLE &&
           sampler() != VK_NULL_HANDLE;
  }

private:
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  CubemapDesc desc_{};
  std::unique_ptr<resource::Image> image_;
  std::unique_ptr<resource::ImageView> image_view_;
  std::unique_ptr<resource::Sampler> sampler_;

  void create();
};

} // namespace vkr::scene
