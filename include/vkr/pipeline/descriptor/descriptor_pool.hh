#pragma once
#include "../../ctx.hh"
#include <vector>

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
  DescriptorPool(VkDevice device, uint32_t maxSets,
                 const DescriptorPoolSizes &sizes);
  DescriptorPool(const VulkanContext &ctx, uint32_t maxSets,
                 const DescriptorPoolSizes &sizes);
  ~DescriptorPool();

  DescriptorPool(const DescriptorPool &) = delete;
  DescriptorPool &operator=(const DescriptorPool &) = delete;

  DescriptorPool(DescriptorPool &&other) noexcept;
  DescriptorPool &operator=(DescriptorPool &&other) noexcept;

  [[nodiscard]] VkDescriptorPool pool() const noexcept { return _pool; }
  [[nodiscard]] bool canAllocate() const noexcept;

  void reset();

private:
  VkDevice device{VK_NULL_HANDLE};
  VkDescriptorPool _pool{VK_NULL_HANDLE};
  uint32_t _maxSets{0};
  uint32_t _allocatedSets{0};

  void cleanup();
};

} // namespace vkr
