#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include <cstdint>
#include <vector>

namespace vkr::pipeline {

class DescriptorSet {
public:
  explicit DescriptorSet(const core::Device &device, const DescriptorPool &pool,
                         const DescriptorSetLayout &layout);
  ~DescriptorSet();

  DescriptorSet(const DescriptorSet &) = delete;
  auto operator=(const DescriptorSet &) -> DescriptorSet & = delete;

  DescriptorSet(DescriptorSet &&other) noexcept;
  auto operator=(DescriptorSet &&) -> DescriptorSet & = delete;

  void create();
  void destroy() noexcept;
  void update();

  template <typename T>
  auto uniform(uint32_t binding, T &buffer, uint32_t resourceIndex = 0)
      -> DescriptorSet & {
    assign(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
           uniformInfo(buffer, resourceIndex, 0));
    return *this;
  }

  template <typename T>
  auto storage(uint32_t binding, T &buffer) -> DescriptorSet & {
    assign(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
           buffer.descriptorInfo(0, buffer.bufferSize()));
    return *this;
  }

  template <typename T>
  auto texture(uint32_t binding, T &texture) -> DescriptorSet & {
    assign(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           texture.descriptorInfo());
    return *this;
  }

  [[nodiscard]] auto set() const noexcept -> VkDescriptorSet { return set_; }
  [[nodiscard]] auto valid() const noexcept -> bool {
    return set_ != VK_NULL_HANDLE;
  }

private:
  template <typename Info> struct Binding {
    uint32_t binding;
    VkDescriptorType type;
    Info info{};
    bool dirty{true};
  };

  // dependencies
  const core::Device &device_;
  const DescriptorPool &pool_;
  const DescriptorSetLayout &layout_;

  // components
  VkDescriptorSet set_{VK_NULL_HANDLE};
  std::vector<Binding<VkDescriptorBufferInfo>> buffers_{};
  std::vector<Binding<VkDescriptorImageInfo>> images_{};
  std::vector<VkWriteDescriptorSet> writes_{};

  template <typename T>
  [[nodiscard]] static auto uniformInfo(T &buffer, uint32_t, int64_t)
      -> decltype(buffer.descriptorInfo()) {
    return buffer.descriptorInfo();
  }

  template <typename T>
  [[nodiscard]] static auto uniformInfo(T &buffers, uint32_t resourceIndex, int)
      -> decltype(buffers.frameCount(), buffers.descriptorInfo(resourceIndex)) {
    return buffers.descriptorInfo(resourceIndex);
  }

  void assign(uint32_t binding, VkDescriptorType type,
              VkDescriptorBufferInfo info);
  void assign(uint32_t binding, VkDescriptorType type,
              VkDescriptorImageInfo info);
  void validateBinding(uint32_t binding, VkDescriptorType type) const;
};

} // namespace vkr::pipeline
