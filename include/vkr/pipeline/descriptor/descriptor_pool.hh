#pragma once

#include "../../core/device.hh"

namespace vkr {

struct DescriptorPoolSizes {
  uint32_t uniformBufferCount{0};
  uint32_t storageBufferCount{0};
  uint32_t combinedImageSamplerCount{0};
  uint32_t storageImageCount{0};
  uint32_t inputAttachmentCount{0};
};

class DescriptorPool {
public:
  explicit DescriptorPool(const Device &device, uint32_t maxSets,
                          const DescriptorPoolSizes &sizes);
  ~DescriptorPool();

  DescriptorPool(const DescriptorPool &) = delete;
  DescriptorPool &operator=(const DescriptorPool &) = delete;

  [[nodiscard]] VkDescriptorPool pool() const noexcept { return _pool; }
  [[nodiscard]] bool canAllocate() const noexcept;

  void reset();

private:
  // dependencies
  const Device &device;

  // components
  VkDescriptorPool _pool{VK_NULL_HANDLE};
  uint32_t _maxSets{0};
  uint32_t _allocatedSets{0};

  void cleanup();
};

} // namespace vkr
