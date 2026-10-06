#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::pipeline {

struct DescriptorBufferWrite {
  uint32_t binding{0};
  uint32_t arrayElement{0};
  VkDescriptorType type{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
  std::vector<VkDescriptorBufferInfo> buffers{};

  [[nodiscard]] static auto one(uint32_t binding, VkDescriptorType type,
                                VkDescriptorBufferInfo buffer,
                                uint32_t arrayElement = 0)
      -> DescriptorBufferWrite {
    DescriptorBufferWrite write{};
    write.binding = binding;
    write.arrayElement = arrayElement;
    write.type = type;
    write.buffers.push_back(buffer);
    return write;
  }

  [[nodiscard]] static auto uniform(uint32_t binding,
                                    VkDescriptorBufferInfo buffer,
                                    uint32_t arrayElement = 0)
      -> DescriptorBufferWrite {
    return one(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer,
               arrayElement);
  }

  [[nodiscard]] static auto storage(uint32_t binding,
                                    VkDescriptorBufferInfo buffer,
                                    uint32_t arrayElement = 0)
      -> DescriptorBufferWrite {
    return one(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer,
               arrayElement);
  }

  [[nodiscard]] auto descriptorCount() const noexcept -> uint32_t {
    return static_cast<uint32_t>(buffers.size());
  }
};

struct DescriptorImageWrite {
  uint32_t binding{0};
  uint32_t arrayElement{0};
  VkDescriptorType type{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
  std::vector<VkDescriptorImageInfo> images{};

  [[nodiscard]] static auto one(uint32_t binding, VkDescriptorType type,
                                VkDescriptorImageInfo image,
                                uint32_t arrayElement = 0)
      -> DescriptorImageWrite {
    DescriptorImageWrite write{};
    write.binding = binding;
    write.arrayElement = arrayElement;
    write.type = type;
    write.images.push_back(image);
    return write;
  }

  [[nodiscard]] static auto combinedImageSampler(uint32_t binding,
                                                 VkDescriptorImageInfo image,
                                                 uint32_t arrayElement = 0)
      -> DescriptorImageWrite {
    return one(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image,
               arrayElement);
  }

  [[nodiscard]] static auto sampled(uint32_t binding,
                                    VkDescriptorImageInfo image,
                                    uint32_t arrayElement = 0)
      -> DescriptorImageWrite {
    return one(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, image, arrayElement);
  }

  [[nodiscard]] static auto storage(uint32_t binding,
                                    VkDescriptorImageInfo image,
                                    uint32_t arrayElement = 0)
      -> DescriptorImageWrite {
    return one(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, image, arrayElement);
  }

  [[nodiscard]] auto descriptorCount() const noexcept -> uint32_t {
    return static_cast<uint32_t>(images.size());
  }
};

struct DescriptorSetWrite {
  uint32_t setIndex{0};
  std::vector<DescriptorBufferWrite> buffers{};
  std::vector<DescriptorImageWrite> images{};

  [[nodiscard]] static auto forSet(uint32_t setIndex) -> DescriptorSetWrite {
    DescriptorSetWrite write{};
    write.setIndex = setIndex;
    return write;
  }

  [[nodiscard]] auto empty() const noexcept -> bool {
    return buffers.empty() && images.empty();
  }
};

} // namespace vkr::pipeline

