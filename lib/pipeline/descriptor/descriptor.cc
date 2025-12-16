#include "vkr/pipeline/descriptor/descriptor.hh"
#include <stdexcept>

namespace vkr {

DescriptorSets::DescriptorSets(const Device &device,
                               DescriptorSetLayout &layout,
                               const DescriptorPool &pool, uint32_t frameCount)
    : device(device), layout(layout), pool(pool), _frameCount(frameCount) {
  allocateSets();
}

DescriptorSets::~DescriptorSets() {
  if (!_sets.empty()) {
    vkFreeDescriptorSets(device.device(), pool.pool(),
                         static_cast<uint32_t>(_sets.size()), _sets.data());
  }
}

void DescriptorSets::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(_frameCount, layout.layout());

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool.pool();
  allocInfo.descriptorSetCount = _frameCount;
  allocInfo.pSetLayouts = layouts.data();

  _sets.resize(_frameCount);
  VkResult result =
      vkAllocateDescriptorSets(device.device(), &allocInfo, _sets.data());
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets.  VkResult: " +
                             std::to_string(result));
  }
}

void DescriptorSets::bindUniformBuffer(uint32_t binding,
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

    DescriptorWriter writer(device);
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
    writer.update(_sets[i]);
  }
}

void DescriptorSets::bindStorageBuffer(uint32_t binding,
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

    DescriptorWriter writer(device);
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo);
    writer.update(_sets[i]);
  }
}

void DescriptorSets::bindImageSampler(
    uint32_t binding, const std::vector<VkImageView> &imageViews,
    VkSampler sampler, VkImageLayout layout) {
  if (imageViews.size() != _frameCount) {
    throw std::runtime_error("Image view count must match frame count");
  }

  for (uint32_t i = 0; i < _frameCount; ++i) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = layout;
    imageInfo.imageView = imageViews[i];
    imageInfo.sampler = sampler;

    DescriptorWriter writer(device);
    writer.writeImage(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      &imageInfo);
    writer.update(_sets[i]);
  }
}

void DescriptorSets::bindToFrame(uint32_t frameIndex,
                                 DescriptorWriter &writer) {
  if (frameIndex >= _frameCount) {
    throw std::runtime_error("Frame index out of bounds");
  }
  writer.update(_sets[frameIndex]);
}

void DescriptorSets::bind(VkCommandBuffer cmd, VkPipelineLayout layout,
                          uint32_t frameIndex, VkPipelineBindPoint bindPoint) {
  if (frameIndex >= _frameCount) {
    throw std::runtime_error("Frame index out of bounds");
  }
  vkCmdBindDescriptorSets(cmd, bindPoint, layout, 0, 1, &_sets[frameIndex], 0,
                          nullptr);
}

DescriptorManager::DescriptorManager(const Device &device) : device(device) {}

DescriptorPoolSizes DescriptorManager::calculatePoolSizes(uint32_t maxSets) {
  DescriptorPoolSizes sizes;
  sizes.uniformBufferCount = maxSets * 10;
  sizes.storageBufferCount = maxSets * 10;
  sizes.combinedImageSamplerCount = maxSets * 10;
  sizes.storageImageCount = maxSets * 10;
  sizes.inputAttachmentCount = maxSets * 10;
  return sizes;
}

std::shared_ptr<DescriptorSetLayout> DescriptorManager::createLayout(
    const std::vector<DescriptorBinding> &bindings) {
  return std::make_shared<DescriptorSetLayout>(device, bindings);
}

std::unique_ptr<DescriptorSets>
DescriptorManager::allocate(DescriptorSetLayout &layout,
                            const DescriptorPool &pool, uint32_t frameCount) {
  return std::make_unique<DescriptorSets>(device, layout, pool, frameCount);
}

} // namespace vkr
