#pragma once

#include "../../core/device.hh"

namespace vkr::resource {
class Sampler {
public:
  explicit Sampler(const core::Device &device);
  ~Sampler();

  Sampler(const Sampler &) = delete;
  auto operator=(const Sampler &) -> Sampler & = delete;

  [[nodiscard]] auto sampler() const noexcept -> VkSampler {
    return _vk_sampler_;
  }

private:
  // dependeies
  const core::Device &device_;

  // components
  VkSampler _vk_sampler_{VK_NULL_HANDLE};
};
} // namespace vkr::resource
