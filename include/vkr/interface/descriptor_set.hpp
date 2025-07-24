#pragma once

#include "../ctx.hpp"

namespace vkr {

class DescriptorSet {
public:
  DescriptorSet(VkDevice device, VkDescriptorSetLayout layout,
                std::vector<VkBuffer> uniformBuffers);
  DescriptorSet(const VulkanContext &ctx);
  ~DescriptorSet();

  DescriptorSet(const DescriptorSet &) = delete;
  DescriptorSet &operator=(const DescriptorSet &) = delete;

  [[nodiscard]] std::vector<VkDescriptorSet> descriptorSets() const noexcept {
    return _descriptorSets;
  }
  [[nodiscard]] VkDescriptorPool descriptorPool() const {
    return _descriptorPool;
  }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};
  std::vector<VkBuffer> uniformBuffers{VK_NULL_HANDLE};

  // components
  std::vector<VkDescriptorSet> _descriptorSets{};
  VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
};
} // namespace vkr
