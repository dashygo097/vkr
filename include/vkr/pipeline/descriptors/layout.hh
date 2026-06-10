#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/binding.hh"

namespace vkr::pipeline {

class DescriptorSetLayout {
public:
  explicit DescriptorSetLayout(const core::Device &device,
                               const std::vector<DescriptorBinding> &bindings);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  auto operator=(const DescriptorSetLayout &) -> DescriptorSetLayout & = delete;

  [[nodiscard]] auto bindings() const noexcept
      -> const std::vector<DescriptorBinding> & {
    return bindings_;
  }

  [[nodiscard]] auto layoutRef() noexcept -> VkDescriptorSetLayout & {
    return layout_;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  std::vector<DescriptorBinding> bindings_{};
  VkDescriptorSetLayout layout_{VK_NULL_HANDLE};
};

} // namespace vkr::pipeline
