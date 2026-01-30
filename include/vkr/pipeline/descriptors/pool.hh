#pragma once

#include "../../core/device.hh"

namespace vkr::pipeline {

struct DescriptorPoolSizes {
  uint32_t uniformBufferCount{0};
  uint32_t storageBufferCount{0};
  uint32_t combinedImageSamplerCount{0};
  uint32_t storageImageCount{0};
  uint32_t inputAttachmentCount{0};
};

class DescriptorPool {
public:
  explicit DescriptorPool(const core::Device &device, uint32_t maxSets,
                          const DescriptorPoolSizes &sizes);
  ~DescriptorPool();

  DescriptorPool(const DescriptorPool &) = delete;
  DescriptorPool &operator=(const DescriptorPool &) = delete;

  [[nodiscard]] VkDescriptorPool pool() const noexcept { return pool_; }
  [[nodiscard]] bool canAllocate() const noexcept;

  void reset();

private:
  // dependencies
  const core::Device &device_;
  uint32_t max_sets_{0};

  // components
  VkDescriptorPool pool_{VK_NULL_HANDLE};
  uint32_t allocated_sets_{0};

  void cleanup();
};

} // namespace vkr::pipeline
