#pragma once

#include "vkr/core/device.hh"
#include <vector>

namespace vkr::pipeline {

struct DescriptorBufferWriteDesc {
  uint32_t binding{0};
  uint32_t arrayElement{0};
  VkDescriptorType type{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
  std::vector<VkDescriptorBufferInfo> buffers{};

  [[nodiscard]] static auto one(uint32_t binding, VkDescriptorType type,
                                VkDescriptorBufferInfo buffer,
                                uint32_t arrayElement = 0)
      -> DescriptorBufferWriteDesc {
    DescriptorBufferWriteDesc desc{};
    desc.binding = binding;
    desc.arrayElement = arrayElement;
    desc.type = type;
    desc.buffers.push_back(buffer);
    return desc;
  }

  [[nodiscard]] static auto uniform(uint32_t binding,
                                    VkDescriptorBufferInfo buffer,
                                    uint32_t arrayElement = 0)
      -> DescriptorBufferWriteDesc {
    return one(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer,
               arrayElement);
  }

  [[nodiscard]] static auto storage(uint32_t binding,
                                    VkDescriptorBufferInfo buffer,
                                    uint32_t arrayElement = 0)
      -> DescriptorBufferWriteDesc {
    return one(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer,
               arrayElement);
  }

  [[nodiscard]] auto descriptorCount() const noexcept -> uint32_t {
    return static_cast<uint32_t>(buffers.size());
  }
};

struct DescriptorImageWriteDesc {
  uint32_t binding{0};
  uint32_t arrayElement{0};
  VkDescriptorType type{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
  std::vector<VkDescriptorImageInfo> images{};

  [[nodiscard]] static auto one(uint32_t binding, VkDescriptorType type,
                                VkDescriptorImageInfo image,
                                uint32_t arrayElement = 0)
      -> DescriptorImageWriteDesc {
    DescriptorImageWriteDesc desc{};
    desc.binding = binding;
    desc.arrayElement = arrayElement;
    desc.type = type;
    desc.images.push_back(image);
    return desc;
  }

  [[nodiscard]] static auto combinedImageSampler(
      uint32_t binding, VkDescriptorImageInfo image,
      uint32_t arrayElement = 0) -> DescriptorImageWriteDesc {
    return one(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image,
               arrayElement);
  }

  [[nodiscard]] static auto sampled(uint32_t binding,
                                    VkDescriptorImageInfo image,
                                    uint32_t arrayElement = 0)
      -> DescriptorImageWriteDesc {
    return one(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, image, arrayElement);
  }

  [[nodiscard]] static auto storage(uint32_t binding,
                                    VkDescriptorImageInfo image,
                                    uint32_t arrayElement = 0)
      -> DescriptorImageWriteDesc {
    return one(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, image, arrayElement);
  }

  [[nodiscard]] auto descriptorCount() const noexcept -> uint32_t {
    return static_cast<uint32_t>(images.size());
  }
};

struct DescriptorSetWriteDesc {
  uint32_t setIndex{0};
  std::vector<DescriptorBufferWriteDesc> buffers{};
  std::vector<DescriptorImageWriteDesc> images{};

  [[nodiscard]] static auto forSet(uint32_t setIndex)
      -> DescriptorSetWriteDesc {
    DescriptorSetWriteDesc desc{};
    desc.setIndex = setIndex;
    return desc;
  }

  [[nodiscard]] auto empty() const noexcept -> bool {
    return buffers.empty() && images.empty();
  }
};

struct DescriptorSetsDesc {
  VkDescriptorPool pool{VK_NULL_HANDLE};
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};
  uint32_t setCount{0};
  std::vector<DescriptorSetWriteDesc> writes{};

  [[nodiscard]] auto canAllocate() const noexcept -> bool {
    return pool != VK_NULL_HANDLE && layout != VK_NULL_HANDLE && setCount > 0;
  }
};

class DescriptorSets {
public:
  explicit DescriptorSets(const core::Device &device);
  ~DescriptorSets();

  DescriptorSets(const DescriptorSets &) = delete;
  auto operator=(const DescriptorSets &) -> DescriptorSets & = delete;

  DescriptorSets(DescriptorSets &&) = delete;
  auto operator=(DescriptorSets &&) -> DescriptorSets & = delete;

  void create();
  void destroy();
  void update(const DescriptorSetsDesc &desc);

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

  [[nodiscard]] auto valid() const noexcept -> bool {
    return !sets_.empty() && allocated_pool_ != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  DescriptorSetsDesc desc_{};
  VkDescriptorPool allocated_pool_{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> sets_{};

  void allocateSets();
  void updateDescriptors();
};

} // namespace vkr::pipeline
