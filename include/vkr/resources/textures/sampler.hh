#pragma once

#include "../../core/device.hh"

namespace vkr::resource {
class Sampler {
public:
  explicit Sampler(const core::Device &device);
  ~Sampler();

  Sampler(const Sampler &) = delete;
  Sampler &operator=(const Sampler &) = delete;

private:
  // dependeies
  const core::Device &device_;

  // components
};
} // namespace vkr::resource
