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
  auto operator=(const DescriptorPool &) -> DescriptorPool & = delete;

  [[nodiscard]] auto pool() const noexcept -> VkDescriptorPool { return pool_; }
  [[nodiscard]] auto canAllocate() const noexcept -> bool;

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
