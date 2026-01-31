#pragma once

#include "../../core/device.hh"

namespace vkr::resource {
class Sampler {
public:
  explicit Sampler(const core::Device &device);
  ~Sampler();

  Sampler(const Sampler &) = delete;
  Sampler &operator=(const Sampler &) = delete;

  [[nodiscard]] VkSampler sampler() const noexcept { return _vk_sampler_; }

private:
  // dependeies
  const core::Device &device_;

  // components
  VkSampler _vk_sampler_{VK_NULL_HANDLE};
};
} // namespace vkr::resource
