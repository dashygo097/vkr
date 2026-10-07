#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/binding.hh"
#include <cstddef>
#include <vector>

namespace vkr::pipeline {

struct DescriptorSetLayoutDesc {
  std::vector<DescriptorBinding> bindings{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    for (std::size_t index = 0; index < bindings.size(); ++index) {
      const auto &binding = bindings[index].layout;
      if (binding.descriptorCount == 0 || binding.stageFlags == 0) {
        return false;
      }
      for (std::size_t previous = 0; previous < index; ++previous) {
        if (bindings[previous].layout.binding == binding.binding) {
          return false;
        }
      }
    }
    return true;
  }
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
