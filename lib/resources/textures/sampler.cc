#include "vkr/resources/textures/sampler.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

Sampler::Sampler(const core::Device &device) : device_(device) {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.physicalDevice(), &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(device_.device(), &samplerInfo, nullptr, &_vk_sampler_) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to create texture sampler!");
  }
}

Sampler::~Sampler() {
  if (_vk_sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(device_.device(), _vk_sampler_, nullptr);
    _vk_sampler_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::resource
