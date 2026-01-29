#include "vkr/pipeline/descriptor/descriptor_writer.hh"

namespace vkr::pipeline {

DescriptorWriter::DescriptorWriter(const core::Device &device)
    : device(device) {}

DescriptorWriter &
DescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorType type,
                              const VkDescriptorBufferInfo *bufferInfo) {
  _bufferInfos.push_back(*bufferInfo);

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = 1;
  write.pBufferInfo = &_bufferInfos.back();

  _writes.push_back(write);
  return *this;
}

DescriptorWriter &
DescriptorWriter::writeImage(uint32_t binding, VkDescriptorType type,
                             const VkDescriptorImageInfo *imageInfo) {
  _imageInfos.push_back(*imageInfo);

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = 1;
  write.pImageInfo = &_imageInfos.back();

  _writes.push_back(write);
  return *this;
}

DescriptorWriter &DescriptorWriter::writeBufferArray(
    uint32_t binding, VkDescriptorType type,
    const std::vector<VkDescriptorBufferInfo> &bufferInfos) {

  size_t startIdx = _bufferInfos.size();
  _bufferInfos.insert(_bufferInfos.end(), bufferInfos.begin(),
                      bufferInfos.end());

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = static_cast<uint32_t>(bufferInfos.size());
  write.pBufferInfo = &_bufferInfos[startIdx];

  _writes.push_back(write);
  return *this;
}

DescriptorWriter &DescriptorWriter::writeImageArray(
    uint32_t binding, VkDescriptorType type,
    const std::vector<VkDescriptorImageInfo> &imageInfos) {

  size_t startIdx = _imageInfos.size();
  _imageInfos.insert(_imageInfos.end(), imageInfos.begin(), imageInfos.end());

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = type;
  write.descriptorCount = static_cast<uint32_t>(imageInfos.size());
  write.pImageInfo = &_imageInfos[startIdx];

  _writes.push_back(write);
  return *this;
}

void DescriptorWriter::update(VkDescriptorSet set) {
  for (auto &write : _writes) {
    write.dstSet = set;
  }

  vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(_writes.size()),
                         _writes.data(), 0, nullptr);
}

void DescriptorWriter::clear() {
  _writes.clear();
  _bufferInfos.clear();
  _imageInfos.clear();
}

} // namespace vkr::pipeline
