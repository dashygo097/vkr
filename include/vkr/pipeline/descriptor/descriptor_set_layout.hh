#pragma once

#include "../../ctx.hh"

namespace vkr {

class DescriptorSetLayout {
public:
  DescriptorSetLayout(VkDevice device);
  DescriptorSetLayout(const VulkanContext &ctx);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

  [[nodiscard]] VkDescriptorSetLayout descriptorSetLayout() const noexcept {
    return _descriptorSetLayout;
  }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};

  // components
  VkDescriptorSetLayout _descriptorSetLayout{VK_NULL_HANDLE};
};
} // namespace vkr
