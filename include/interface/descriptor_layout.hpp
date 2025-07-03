#pragma once

#include <vulkan/vulkan.hpp>

#include "ctx.hpp"

class DescriptorSetLayout {
public:
  DescriptorSetLayout(VkDevice device);
  DescriptorSetLayout(const VulkanContext &ctx);
  ~DescriptorSetLayout();

  VkDescriptorSetLayout getVkDescriptorSetLayout() const {
    return descriptorSetLayout;
  }

private:
  // dependencies
  VkDevice device;

  // components
  VkDescriptorSetLayout descriptorSetLayout;
};
