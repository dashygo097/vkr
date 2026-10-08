#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/write.hh"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <variant>
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

  template <typename T>
  auto uniform(uint32_t binding, T &buffer)
      -> decltype(buffer.descriptorInfo(), std::declval<DescriptorSets &>()) {
    bind(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         BufferInfo{[&buffer](uint32_t) { return buffer.descriptorInfo(); }});
    return *this;
  }

  template <typename T>
  auto uniform(uint32_t binding, T &buffers)
      -> decltype(buffers.frameCount(), buffers.descriptorInfo(uint32_t{}),
                  std::declval<DescriptorSets &>()) {
    bind(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         BufferInfo{[&buffers](uint32_t index) {
           return buffers.descriptorInfo(index);
         }});
    return *this;
  }

  template <typename T>
  auto storage(uint32_t binding, T &buffer) -> DescriptorSets & {
    bind(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         BufferInfo{[&buffer](uint32_t) {
           return buffer.descriptorInfo(0, buffer.bufferSize());
         }});
    return *this;
  }

  template <typename T>
  auto texture(uint32_t binding, T &texture) -> DescriptorSets & {
    bind(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         ImageInfo{[&texture](uint32_t) { return texture.descriptorInfo(); }});
    return *this;
  }

  void write(uint32_t setIndex);

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
  using BufferInfo = std::function<VkDescriptorBufferInfo(uint32_t)>;
  using ImageInfo = std::function<VkDescriptorImageInfo(uint32_t)>;

  struct Binding {
    uint32_t binding;
    VkDescriptorType type;
    std::variant<BufferInfo, ImageInfo> info;
  };

  // dependencies
  const core::Device &device_;
  const DescriptorPool &pool_;
  const DescriptorSetLayout &layout_;

  // components
  DescriptorSetsDesc desc_{};
  std::vector<VkDescriptorSet> sets_{};
  std::vector<Binding> bindings_{};
  uint64_t binding_revision_{1};
  std::vector<uint64_t> written_revisions_{};
  std::vector<std::vector<uint8_t>> initialized_{};

  void allocateSets();
  void apply(const std::vector<DescriptorSetWrite> &writes);
  void bind(uint32_t binding, VkDescriptorType type,
            std::variant<BufferInfo, ImageInfo> info);
  [[nodiscard]] auto bindingIndex(uint32_t binding, VkDescriptorType type,
                                  uint32_t arrayElement, uint32_t count) const
      -> size_t;
};

} // namespace vkr::pipeline
