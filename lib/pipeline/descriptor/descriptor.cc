#include "vkr/pipeline/descriptor/descriptor.hh"
#include <stdexcept>

namespace vkr {

Descriptor::Descriptor(VkDevice device, const DescriptorSetLayout &layout,
                       DescriptorPool &pool, uint32_t frameCount)
    : device(device), _pool(&pool), _frameCount(frameCount) {
  allocateSets(layout.layout());
}

Descriptor::Descriptor(const VulkanContext &ctx,
                       const DescriptorSetLayout &layout, DescriptorPool &pool)
    : Descriptor(ctx.device, layout, pool, MAX_FRAMES_IN_FLIGHT) {}

Descriptor::~Descriptor() {
  if (!_sets.empty() && _pool != nullptr && _pool->pool() != VK_NULL_HANDLE) {
    vkFreeDescriptorSets(device, _pool->pool(),
                         static_cast<uint32_t>(_sets.size()), _sets.data());
  }
}

void Descriptor::allocateSets(VkDescriptorSetLayout layout) {
  std::vector<VkDescriptorSetLayout> layouts(_frameCount, layout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _pool->pool();
  allocInfo.descriptorSetCount = _frameCount;
  allocInfo.pSetLayouts = layouts.data();

  _sets.resize(_frameCount);
  VkResult result = vkAllocateDescriptorSets(device, &allocInfo, _sets.data());
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets. VkResult: " +
                             std::to_string(result));
  }
}

void Descriptor::bindUniformBuffer(uint32_t binding,
                                   const std::vector<VkBuffer> &buffers,
                                   VkDeviceSize size, VkDeviceSize offset) {
  if (buffers.size() != _frameCount) {
    throw std::runtime_error("Buffer count must match frame count");
  }

  for (uint32_t i = 0; i < _frameCount; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[i];
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    DescriptorWriter writer;
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
    writer.update(device, _sets[i]);
  }
}

void Descriptor::bindStorageBuffer(uint32_t binding,
                                   const std::vector<VkBuffer> &buffers,
                                   VkDeviceSize size, VkDeviceSize offset) {
  if (buffers.size() != _frameCount) {
    throw std::runtime_error("Buffer count must match frame count");
  }

  for (uint32_t i = 0; i < _frameCount; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[i];
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    DescriptorWriter writer;
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo);
    writer.update(device, _sets[i]);
  }
}

void Descriptor::bindImageSampler(uint32_t binding,
                                  const std::vector<VkImageView> &imageViews,
                                  VkSampler sampler, VkImageLayout layout) {
  if (imageViews.size() != _frameCount) {
    throw std::runtime_error("Image view count must match frame count");
  }

  for (uint32_t i = 0; i < _frameCount; ++i) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = layout;
    imageInfo.imageView = imageViews[i];
    imageInfo.sampler = sampler;

    DescriptorWriter writer;
    writer.writeImage(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      &imageInfo);
    writer.update(device, _sets[i]);
  }
}

void Descriptor::bindToFrame(uint32_t frameIndex, DescriptorWriter &writer) {
  if (frameIndex >= _frameCount) {
    throw std::runtime_error("Frame index out of bounds");
  }
  writer.update(device, _sets[frameIndex]);
}

void Descriptor::bind(VkCommandBuffer cmd, VkPipelineLayout layout,
                      uint32_t frameIndex, VkPipelineBindPoint bindPoint) {
  vkCmdBindDescriptorSets(cmd, bindPoint, layout, 0, 1, &_sets[frameIndex], 0,
                          nullptr);
}

DescriptorManager::DescriptorManager(VkDevice device, uint32_t maxSets)
    : device(device), _pool(device, maxSets, calculatePoolSizes(maxSets)) {}

DescriptorManager::DescriptorManager(const VulkanContext &ctx, uint32_t maxSets)
    : DescriptorManager(ctx.device, maxSets) {}

DescriptorPoolSizes DescriptorManager::calculatePoolSizes(uint32_t maxSets) {
  DescriptorPoolSizes sizes;
  sizes.uniformBufferCount = maxSets;
  sizes.storageBufferCount = maxSets;
  sizes.combinedImageSamplerCount = maxSets;
  sizes.storageImageCount = maxSets / 2;
  sizes.inputAttachmentCount = maxSets / 4;
  return sizes;
}

std::shared_ptr<DescriptorSetLayout> DescriptorManager::createLayout(
    const std::vector<DescriptorBinding> &bindings) {
  return std::make_shared<DescriptorSetLayout>(device, bindings);
}

std::unique_ptr<Descriptor>
DescriptorManager::allocate(const DescriptorSetLayout &layout,
                            uint32_t frameCount) {
  return std::make_unique<Descriptor>(device, layout, _pool, frameCount);
}

void DescriptorManager::reset() { _pool.reset(); }

} // namespace vkr
