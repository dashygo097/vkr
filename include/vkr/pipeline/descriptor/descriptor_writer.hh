#pragma once

#include "../../core/device.hh"

namespace vkr {

class DescriptorWriter {
public:
  explicit DescriptorWriter(const Device &device);

  DescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorType type,
                                const VkDescriptorBufferInfo *bufferInfo);

  DescriptorWriter &writeImage(uint32_t binding, VkDescriptorType type,
                               const VkDescriptorImageInfo *imageInfo);

  DescriptorWriter &
  writeBufferArray(uint32_t binding, VkDescriptorType type,
                   const std::vector<VkDescriptorBufferInfo> &bufferInfos);

  DescriptorWriter &
  writeImageArray(uint32_t binding, VkDescriptorType type,
                  const std::vector<VkDescriptorImageInfo> &imageInfos);

  void update(VkDescriptorSet set);
  void clear();

private:
  // dependencies
  const Device &device;

  // components
  std::vector<VkWriteDescriptorSet> _writes;
  std::vector<VkDescriptorBufferInfo> _bufferInfos;
  std::vector<VkDescriptorImageInfo> _imageInfos;
};

} // namespace vkr
