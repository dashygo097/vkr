#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/binding.hh"
#include <vector>

namespace vkr::pipeline {

struct DescriptorSetLayoutDesc {
  std::vector<DescriptorBinding> bindings{};
};

class DescriptorSetLayout {
public:
  explicit DescriptorSetLayout(const core::Device &device);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  auto operator=(const DescriptorSetLayout &) -> DescriptorSetLayout & = delete;

  DescriptorSetLayout(DescriptorSetLayout &&) = delete;
  auto operator=(DescriptorSetLayout &&) -> DescriptorSetLayout & = delete;

  void create();
  void destroy();
  void update(const DescriptorSetLayoutDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const DescriptorSetLayoutDesc & {
    return desc_;
  }

  [[nodiscard]] auto layout() const noexcept -> VkDescriptorSetLayout {
    return layout_;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return layout_ != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  DescriptorSetLayoutDesc desc_{};
  VkDescriptorSetLayout layout_{VK_NULL_HANDLE};
};

} // namespace vkr::pipeline
