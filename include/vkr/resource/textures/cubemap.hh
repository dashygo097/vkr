#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/sampler.hh"
#include "vkr/resource/texture.hh"
#include <array>
#include <optional>
#include <string>

namespace vkr::res {

struct CubemapDesc {
  std::array<std::string, 6> facePaths{};
  VkFormat format{VK_FORMAT_R8G8B8A8_SRGB};
  VkImageLayout layout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  SamplerDesc sampler{SamplerDesc::linearClampToEdge()};
  bool forceRgba{true};
  bool createSampler{true};

  [[nodiscard]] static auto
  files(const std::array<std::string, 6> &faces,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        SamplerDesc sampler = SamplerDesc::linearClampToEdge()) -> CubemapDesc {
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

class Cubemap final : public Texture {
public:
  explicit Cubemap(const core::Device &device,
                   const core::CommandPool &commandPool);
  ~Cubemap() override = default;

  void destroy() noexcept override;
  void update(const CubemapDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const CubemapDesc & {
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
  CubemapDesc desc_{};
  std::optional<Sampler> sampler_{};

  // helpers
  void create();
};

} // namespace vkr::res
