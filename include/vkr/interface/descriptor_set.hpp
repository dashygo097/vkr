#pragma once

#include "../ctx.hpp"

class DescriptorSet {
public:
  DescriptorSet(VkDevice device, VkDescriptorSetLayout layout,
                std::vector<VkBuffer> uniformBuffers);
  DescriptorSet(const VulkanContext &ctx);
  ~DescriptorSet();

  DescriptorSet(const DescriptorSet &) = delete;
  DescriptorSet &operator=(const DescriptorSet &) = delete;

  std::vector<VkDescriptorSet> getVkDescriptorSets() const {
    return descriptorSets;
  }
  VkDescriptorPool getVkDescriptorPool() const { return descriptorPool; }

private:
  // dependencies
  VkDevice device;
  VkDescriptorSetLayout layout;
  std::vector<VkBuffer> uniformBuffers;

  // components
  std::vector<VkDescriptorSet> descriptorSets{};
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};
