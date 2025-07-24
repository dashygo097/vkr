#include "vkr/interface/descriptor_set.hpp"
#include "vkr/buffers/command.hpp"
#include "vkr/buffers/uniform.hpp"

namespace vkr {

DescriptorSet::DescriptorSet(VkDevice device, VkDescriptorSetLayout layout,
                             std::vector<VkBuffer> uniformBuffers)
    : device(device), layout(layout), uniformBuffers(uniformBuffers) {

  std::vector<VkDescriptorPoolSize> poolSizes{};

  VkDescriptorPoolSize uniformPoolSize{};
  uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes.push_back(uniformPoolSize);

  VkDescriptorPoolSize samplerPoolSize{};
  samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes.push_back(samplerPoolSize);

  VkDescriptorPoolSize storagePoolSize{};
  storagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  storagePoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes.push_back(storagePoolSize);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);

  VkResult result =
      vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool. VkResult: " +
                             std::to_string(result));
  }

  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  result = vkAllocateDescriptorSets(device, &allocInfo, _descriptorSets.data());
  if (result != VK_SUCCESS) {
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
    _descriptorPool = VK_NULL_HANDLE;
    throw std::runtime_error("Failed to allocate descriptor sets. VkResult: " +
                             std::to_string(result));
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = _descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
  }
}

DescriptorSet::DescriptorSet(const VulkanContext &ctx)
    : DescriptorSet(ctx.device, ctx.descriptorSetLayout, ctx.uniformBuffers) {}

DescriptorSet::~DescriptorSet() {
  if (device != VK_NULL_HANDLE && _descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
    _descriptorPool = VK_NULL_HANDLE;
  }
  _descriptorSets.clear();
}
} // namespace vkr
