#include "vkr/pipeline/descriptors/set.hh"
#include <stdexcept>

namespace vkr::pipeline {

DescriptorSets::DescriptorSets(const core::Device &device,
                               DescriptorSetLayout &layout,
                               const DescriptorPool &pool, uint32_t frameCount)
    : device_(device), layout_(layout), pool_(pool), frame_count_(frameCount) {
  allocateSets();
}

DescriptorSets::~DescriptorSets() {
  if (!sets_.empty()) {
    vkFreeDescriptorSets(device_.device(), pool_.pool(),
                         static_cast<uint32_t>(sets_.size()), sets_.data());
  }
}

void DescriptorSets::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(frame_count_, layout_.layout());

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool_.pool();
  allocInfo.descriptorSetCount = frame_count_;
  allocInfo.pSetLayouts = layouts.data();

  sets_.resize(frame_count_);
  VkResult result =
      vkAllocateDescriptorSets(device_.device(), &allocInfo, sets_.data());
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets.  VkResult: " +
                             std::to_string(result));
  }
}

void DescriptorSets::bindUniformBuffer(uint32_t binding,
                                       const std::vector<VkBuffer> &buffers,
                                       VkDeviceSize size, VkDeviceSize offset) {
  if (buffers.size() != frame_count_) {
    throw std::runtime_error("Buffer count must match frame count");
  }

  for (uint32_t i = 0; i < frame_count_; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[i];
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    DescriptorWriter writer(device_);
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
    writer.update(sets_[i]);
  }
}

void DescriptorSets::bindStorageBuffer(uint32_t binding,
                                       const std::vector<VkBuffer> &buffers,
                                       VkDeviceSize size, VkDeviceSize offset) {
  if (buffers.size() != frame_count_) {
    throw std::runtime_error("Buffer count must match frame count");
  }

  for (uint32_t i = 0; i < frame_count_; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[i];
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    DescriptorWriter writer(device_);
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo);
    writer.update(sets_[i]);
  }
}

void DescriptorSets::bindImageSampler(
    uint32_t binding, const std::vector<VkImageView> &imageViews,
    VkSampler sampler, VkImageLayout layout) {
  if (imageViews.size() != frame_count_) {
    throw std::runtime_error("Image view count must match frame count");
  }

  for (uint32_t i = 0; i < frame_count_; ++i) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = layout;
    imageInfo.imageView = imageViews[i];
    imageInfo.sampler = sampler;

    DescriptorWriter writer(device_);
    writer.writeImage(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      &imageInfo);
    writer.update(sets_[i]);
  }
}

void DescriptorSets::bindToFrame(uint32_t frameIndex,
                                 DescriptorWriter &writer) {
  if (frameIndex >= frame_count_) {
    throw std::runtime_error("Frame index out of bounds");
  }
  writer.update(sets_[frameIndex]);
}

void DescriptorSets::bind(VkCommandBuffer cmd, VkPipelineLayout layout,
                          uint32_t frameIndex, VkPipelineBindPoint bindPoint) {
  if (frameIndex >= frame_count_) {
    throw std::runtime_error("Frame index out of bounds");
  }
  vkCmdBindDescriptorSets(cmd, bindPoint, layout, 0, 1, &sets_[frameIndex], 0,
                          nullptr);
}

DescriptorManager::DescriptorManager(const core::Device &device)
    : device_(device) {}

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
  return std::make_shared<DescriptorSetLayout>(device_, bindings);
}

std::unique_ptr<DescriptorSets>
DescriptorManager::allocate(DescriptorSetLayout &layout,
                            const DescriptorPool &pool, uint32_t frameCount) {
  return std::make_unique<DescriptorSets>(device_, layout, pool, frameCount);
}

} // namespace vkr::pipeline
