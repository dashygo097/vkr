#pragma once

#include "vkr/core/device.hh"
#include <vulkan/vulkan.h>

namespace vkr::resource {

struct SamplerDesc {
  VkFilter magFilter{VK_FILTER_LINEAR};
  VkFilter minFilter{VK_FILTER_LINEAR};

  VkSamplerMipmapMode mipmapMode{VK_SAMPLER_MIPMAP_MODE_LINEAR};

  VkSamplerAddressMode addressModeU{VK_SAMPLER_ADDRESS_MODE_REPEAT};
  VkSamplerAddressMode addressModeV{VK_SAMPLER_ADDRESS_MODE_REPEAT};
  VkSamplerAddressMode addressModeW{VK_SAMPLER_ADDRESS_MODE_REPEAT};

  float mipLodBias{0.0F};

  VkBool32 anisotropyEnable{VK_FALSE};
  float maxAnisotropy{1.0F};

  VkBool32 compareEnable{VK_FALSE};
  VkCompareOp compareOp{VK_COMPARE_OP_ALWAYS};

  float minLod{0.0F};
  float maxLod{VK_LOD_CLAMP_NONE};

  VkBorderColor borderColor{VK_BORDER_COLOR_INT_OPAQUE_BLACK};
  VkBool32 unnormalizedCoordinates{VK_FALSE};

  [[nodiscard]] static auto linearRepeat() -> SamplerDesc {
    SamplerDesc desc{};
    desc.magFilter = VK_FILTER_LINEAR;
    desc.minFilter = VK_FILTER_LINEAR;
    desc.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    desc.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.anisotropyEnable = VK_FALSE;
    desc.maxAnisotropy = 1.0F;
    return desc;
  }

  [[nodiscard]] static auto linearClampToEdge() -> SamplerDesc {
    SamplerDesc desc = linearRepeat();
    desc.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    return desc;
  }

  [[nodiscard]] static auto nearestRepeat() -> SamplerDesc {
    SamplerDesc desc = linearRepeat();
    desc.magFilter = VK_FILTER_NEAREST;
    desc.minFilter = VK_FILTER_NEAREST;
    desc.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    return desc;
  }

  [[nodiscard]] static auto nearestClampToEdge() -> SamplerDesc {
    SamplerDesc desc = nearestRepeat();
    desc.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    return desc;
  }

  [[nodiscard]] static auto shadowCompare() -> SamplerDesc {
    SamplerDesc desc = linearClampToEdge();
    desc.compareEnable = VK_TRUE;
    desc.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    desc.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    return desc;
  }

  [[nodiscard]] auto withAnisotropy(float maxValue) const -> SamplerDesc {
    SamplerDesc desc = *this;
    desc.anisotropyEnable = VK_TRUE;
    desc.maxAnisotropy = maxValue;
    return desc;
  }
};

class Sampler {
public:
  explicit Sampler(const core::Device &device);
  ~Sampler();

  Sampler(const Sampler &) = delete;
  auto operator=(const Sampler &) -> Sampler & = delete;

  void create();
  void destroy();
  void update(const SamplerDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const SamplerDesc & {
    return desc_;
  }

  [[nodiscard]] auto sampler() const noexcept -> VkSampler {
    return vk_sampler_;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  SamplerDesc desc_{};
  VkSampler vk_sampler_{VK_NULL_HANDLE};
};

} // namespace vkr::resource
