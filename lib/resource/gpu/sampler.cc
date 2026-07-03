#include "vkr/resource/gpu/sampler.hh"
#include "vkr/logger.hh"
#include <algorithm>

namespace vkr::resource {

Sampler::Sampler(const core::Device &device) : device_(device) {}

Sampler::~Sampler() { destroy(); }

void Sampler::create() {
  destroy();

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.physicalDevice(), &properties);

  VkPhysicalDeviceFeatures features{};
  vkGetPhysicalDeviceFeatures(device_.physicalDevice(), &features);

  SamplerDesc effective = desc_;

  if (effective.anisotropyEnable == VK_TRUE) {
    if (features.samplerAnisotropy == VK_FALSE) {
      VKR_RES_ERROR("Sampler anisotropy requested, but physical device does "
                    "not support samplerAnisotropy");
    }

    effective.maxAnisotropy = std::clamp(
        effective.maxAnisotropy, 1.0f, properties.limits.maxSamplerAnisotropy);
  } else {
    effective.maxAnisotropy = 1.0f;
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = effective.magFilter;
  samplerInfo.minFilter = effective.minFilter;
  samplerInfo.mipmapMode = effective.mipmapMode;
  samplerInfo.addressModeU = effective.addressModeU;
  samplerInfo.addressModeV = effective.addressModeV;
  samplerInfo.addressModeW = effective.addressModeW;
  samplerInfo.mipLodBias = effective.mipLodBias;
  samplerInfo.anisotropyEnable = effective.anisotropyEnable;
  samplerInfo.maxAnisotropy = effective.maxAnisotropy;
  samplerInfo.compareEnable = effective.compareEnable;
  samplerInfo.compareOp = effective.compareOp;
  samplerInfo.minLod = effective.minLod;
  samplerInfo.maxLod = effective.maxLod;
  samplerInfo.borderColor = effective.borderColor;
  samplerInfo.unnormalizedCoordinates = effective.unnormalizedCoordinates;

  if (vkCreateSampler(device_.device(), &samplerInfo, nullptr, &vk_sampler_) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to create sampler");
  }

  desc_ = effective;
}

void Sampler::destroy() {
  if (vk_sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(device_.device(), vk_sampler_, nullptr);
    vk_sampler_ = VK_NULL_HANDLE;
  }
}

void Sampler::update(const SamplerDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

} // namespace vkr::resource
