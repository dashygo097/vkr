#pragma once

#include "../../core/device.hh"

namespace vkr {
class Sampler {
public:
  explicit Sampler(const Device &device);
  ~Sampler();

  Sampler(const Sampler &) = delete;
  Sampler &operator=(const Sampler &) = delete;

private:
  // dependeies
  const Device &device;

  // components
};
} // namespace vkr
