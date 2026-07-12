#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/binding.hh"

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

  void create();
  void destroy();
  void update(const DescriptorSetLayoutDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const DescriptorSetLayoutDesc & {
    return desc_;
  }

  [[nodiscard]] auto bindings() const noexcept
      -> const std::vector<DescriptorBinding> & {
    return desc_.bindings;
  }

  [[nodiscard]] auto layout() const noexcept -> const VkDescriptorSetLayout & {
    return layout_;
  }

  [[nodiscard]] auto layout() noexcept -> VkDescriptorSetLayout & {
    return layout_;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  DescriptorSetLayoutDesc desc_{};
  VkDescriptorSetLayout layout_{VK_NULL_HANDLE};
};

} // namespace vkr::pipeline
