#pragma once
#include "../../ctx.hh"
#include <memory>
#include <vector>

namespace vkr {

class DescriptorWriter {
public:
  DescriptorWriter() = default;

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

  void update(VkDevice device, VkDescriptorSet set);
  void clear();

private:
  std::vector<VkWriteDescriptorSet> _writes;
  std::vector<VkDescriptorBufferInfo> _bufferInfos;
  std::vector<VkDescriptorImageInfo> _imageInfos;
};

} // namespace vkr
