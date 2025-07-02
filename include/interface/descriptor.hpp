#include <vulkan/vulkan.hpp>

#include "ctx.hpp"

class Descriptor {
public:
  Descriptor(VkDevice device);
  Descriptor(const VulkanContext &ctx);
  ~Descriptor();

  VkDescriptorSetLayout getVkDescriptorSetLayout() const {
    return descriptorSetLayout;
  }

private:
  // dependencies
  VkDevice device;

  // components
  VkDescriptorSetLayout descriptorSetLayout;
};
