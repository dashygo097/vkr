#pragma once

#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include <algorithm>
#include <limits>
#include <vector>

namespace vkr::pipeline {

struct DescriptorPoolDesc {
  std::vector<VkDescriptorPoolSize> poolSizes{};
  uint32_t maxSets{0};
  VkDescriptorPoolCreateFlags flags{
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT};

  [[nodiscard]] static auto sets(const DescriptorSetLayoutDesc &layout,
                                 uint32_t count) -> DescriptorPoolDesc {
    if (count == 0) {
      VKR_PIPE_ERROR("Descriptor pool set count must be nonzero");
    }
    DescriptorPoolDesc desc{};
    desc.maxSets = count;
    for (const auto &binding : layout.bindings) {
      auto size =
          std::find_if(desc.poolSizes.begin(), desc.poolSizes.end(),
                       [&binding](const VkDescriptorPoolSize &value) -> bool {
                         return value.type == binding.layout.descriptorType;
                       });
      if (binding.layout.descriptorCount == 0 ||
          binding.layout.descriptorCount >
              std::numeric_limits<uint32_t>::max() / count) {
        VKR_PIPE_ERROR("Invalid descriptor pool size");
      }
      const auto descriptors = binding.layout.descriptorCount * count;
      if (size == desc.poolSizes.end()) {
        desc.poolSizes.push_back({binding.layout.descriptorType, descriptors});
      } else {
        if (descriptors >
            std::numeric_limits<uint32_t>::max() - size->descriptorCount) {
          VKR_PIPE_ERROR("Descriptor pool size overflow");
        }
        size->descriptorCount += descriptors;
      }
    }
    return desc;
  }
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

private:
  // dependencies
  const core::Device &device_;

  // components
  DescriptorPoolDesc desc_{};
  VkDescriptorPool pool_{VK_NULL_HANDLE};
};

} // namespace vkr::pipeline
