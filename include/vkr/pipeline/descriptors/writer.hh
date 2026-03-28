#pragma once

#include "../../core/device.hh"

namespace vkr::pipeline {

class DescriptorWriter {
public:
  explicit DescriptorWriter(const core::Device &device);

  auto writeBuffer(uint32_t binding, VkDescriptorType type,
                                const VkDescriptorBufferInfo *bufferInfo) -> DescriptorWriter &;

  auto writeImage(uint32_t binding, VkDescriptorType type,
                               const VkDescriptorImageInfo *imageInfo) -> DescriptorWriter &;

  auto
  writeBufferArray(uint32_t binding, VkDescriptorType type,
                   const std::vector<VkDescriptorBufferInfo> &bufferInfos) -> DescriptorWriter &;

  auto
  writeImageArray(uint32_t binding, VkDescriptorType type,
                  const std::vector<VkDescriptorImageInfo> &imageInfos) -> DescriptorWriter &;

  void update(VkDescriptorSet set);
  void clear();

private:
  // dependencies
  const core::Device &device_;

  // components
  std::vector<VkWriteDescriptorSet> writes_{};
  std::vector<VkDescriptorBufferInfo> buffer_infos_{};
  std::vector<VkDescriptorImageInfo> image_infos_{};
};

} // namespace vkr::pipeline
