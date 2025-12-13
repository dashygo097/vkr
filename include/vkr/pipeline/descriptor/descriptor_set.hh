#pragma once

#include "../../ctx.hh"

namespace vkr {

class DescriptorSets {
public:
  DescriptorSets(VkDevice device, VkDescriptorSetLayout layout,
                 std::vector<VkBuffer> uniformBuffers);
  DescriptorSets(const VulkanContext &ctx);
  ~DescriptorSets();

  DescriptorSets(const DescriptorSets &) = delete;
  DescriptorSets &operator=(const DescriptorSets &) = delete;

  [[nodiscard]] std::vector<VkDescriptorSet> descriptorSets() const noexcept {
    return _descriptorSets;
  }
  [[nodiscard]] VkDescriptorPool descriptorPool() const noexcept {
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
