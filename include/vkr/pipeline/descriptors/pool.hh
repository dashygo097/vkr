#pragma once

#include "vkr/core/device.hh"

namespace vkr::pipeline {

struct DescriptorPoolDesc {
  std::vector<VkDescriptorPoolSize> poolSizes{};
  uint32_t maxSets{0};
  uint32_t allocatedSets{0};
  VkDescriptorPoolCreateFlags flags{
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT};
};

class DescriptorPool {
public:
  explicit DescriptorPool(const core::Device &device);
  ~DescriptorPool();

  DescriptorPool(const DescriptorPool &) = delete;
  auto operator=(const DescriptorPool &) -> DescriptorPool & = delete;

  DescriptorPool(DescriptorPool &&) = delete;
  auto operator=(DescriptorPool &&) -> DescriptorPool & = delete;

  void create();
  void destroy();
  void update(const DescriptorPoolDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const DescriptorPoolDesc & {
    return desc_;
  }

  [[nodiscard]] auto pool() const noexcept -> VkDescriptorPool { return pool_; }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return pool_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] auto canAllocate() const noexcept -> bool {
    return pool_ != VK_NULL_HANDLE && desc_.allocatedSets < desc_.maxSets;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  DescriptorPoolDesc desc_{};
  VkDescriptorPool pool_{VK_NULL_HANDLE};
};

} // namespace vkr::pipeline
