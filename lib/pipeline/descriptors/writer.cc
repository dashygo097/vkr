#include "vkr/pipeline/descriptors/writer.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

DescriptorWriter::DescriptorWriter(const core::Device &device)
    : device_(device) {}

DescriptorWriter &
DescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorType type,
                              const VkDescriptorBufferInfo *bufferInfo) {
  std::string typeStr;
  switch (type) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    typeStr = "uniform";
    break;
  default:
    typeStr = "unknown";
    break;
  }

  VKR_PIPE_TRACE(
      "Writing buffer descriptor(binding={}, type={}, offset={}, range={})",
      binding, typeStr, bufferInfo->offset, bufferInfo->range);

  buffer_infos_.push_back(*bufferInfo);

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = 1;
  write.pBufferInfo = &buffer_infos_.back();

  writes_.push_back(write);

  return *this;
}

DescriptorWriter &
DescriptorWriter::writeImage(uint32_t binding, VkDescriptorType type,
                             const VkDescriptorImageInfo *imageInfo) {

  std::string typeStr;
  switch (type) {
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
    typeStr = "combined image sampler";
    break;
  default:
    typeStr = "unknown";
    break;
  }

  VKR_PIPE_TRACE("Writing image descriptor(binding={}, type={})", binding,
                 typeStr);

  image_infos_.push_back(*imageInfo);

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = 1;
  write.pImageInfo = &image_infos_.back();

  writes_.push_back(write);
  return *this;
}

DescriptorWriter &DescriptorWriter::writeBufferArray(
    uint32_t binding, VkDescriptorType type,
    const std::vector<VkDescriptorBufferInfo> &bufferInfos) {
  std::string typeStr;
  switch (type) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    typeStr = "uniform";
    break;
  default:
    typeStr = "unknown";
    break;
  }

  VKR_PIPE_TRACE(
      "Writing buffer array descriptor(binding={}, type={}, count={})", binding,
      typeStr, bufferInfos.size());

  size_t startIdx = buffer_infos_.size();
  buffer_infos_.insert(buffer_infos_.end(), bufferInfos.begin(),
                       bufferInfos.end());

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = static_cast<uint32_t>(bufferInfos.size());
  write.pBufferInfo = &buffer_infos_[startIdx];

  writes_.push_back(write);
  return *this;
}

DescriptorWriter &DescriptorWriter::writeImageArray(
    uint32_t binding, VkDescriptorType type,
    const std::vector<VkDescriptorImageInfo> &imageInfos) {
  std::string typeStr;
  switch (type) {
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
    typeStr = "combined image sampler";
    break;
  default:
    typeStr = "unknown";
    break;
  }

  VKR_PIPE_TRACE(
      "Writing image array descriptor(binding={}, type={}, count={})", binding,
      typeStr, imageInfos.size());

  size_t startIdx = image_infos_.size();
  image_infos_.insert(image_infos_.end(), imageInfos.begin(), imageInfos.end());

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = static_cast<uint32_t>(imageInfos.size());
  write.pImageInfo = &image_infos_[startIdx];

  writes_.push_back(write);
  return *this;
}

void DescriptorWriter::update(VkDescriptorSet set) {
  for (auto &write : writes_) {
    write.dstSet = set;
  }

  vkUpdateDescriptorSets(device_.device(),
                         static_cast<uint32_t>(writes_.size()), writes_.data(),
                         0, nullptr);
  VKR_PIPE_INFO("Descriptor set updated with {} writes", writes_.size());
}

void DescriptorWriter::clear() {
  writes_.clear();
  buffer_infos_.clear();
  image_infos_.clear();
}

} // namespace vkr::pipeline
