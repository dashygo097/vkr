#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/write.hh"
#include <vector>

namespace vkr::pipeline {

struct DescriptorSetsDesc {
  uint32_t setCount{0};
};

class DescriptorSets {
public:
  DescriptorSets(const core::Device &device, const DescriptorPool &pool,
                 const DescriptorSetLayout &layout);
  ~DescriptorSets();

  DescriptorSets(const DescriptorSets &) = delete;
  auto operator=(const DescriptorSets &) -> DescriptorSets & = delete;

  DescriptorSets(DescriptorSets &&) = delete;
  auto operator=(DescriptorSets &&) -> DescriptorSets & = delete;

  void create();
  void destroy();
  void update(const DescriptorSetsDesc &desc);
  void write(const std::vector<DescriptorSetWrite> &writes);

  [[nodiscard]] auto desc() const noexcept -> const DescriptorSetsDesc & {
    return desc_;
  }

  [[nodiscard]] auto sets() const noexcept
      -> const std::vector<VkDescriptorSet> & {
    return sets_;
  }

  [[nodiscard]] auto set(uint32_t index) const -> VkDescriptorSet;

  [[nodiscard]] auto count() const noexcept -> uint32_t {
    return static_cast<uint32_t>(sets_.size());
  }

  [[nodiscard]] auto empty() const noexcept -> bool { return sets_.empty(); }

  [[nodiscard]] auto valid() const noexcept -> bool { return !sets_.empty(); }

private:
  // dependencies
  const core::Device &device_;
  const DescriptorPool &pool_;
  const DescriptorSetLayout &layout_;

  // components
  DescriptorSetsDesc desc_{};
  std::vector<VkDescriptorSet> sets_{};

  void allocateSets();
};

} // namespace vkr::pipeline
