#pragma once

#include "../../ctx.hh"
#include <vector>

namespace vkr {
enum class DescriptorType {
  UniformBuffer,
  StorageBuffer,
  CombinedImageSampler,
  StorageImage,
  InputAttachment
};

struct DescriptorBinding {
  uint32_t binding;
  DescriptorType type;
  uint32_t count{1};
  VkShaderStageFlags stageFlags;

  [[nodiscard]] VkDescriptorType toVkType() const noexcept {
    switch (type) {
    case DescriptorType::UniformBuffer:
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::StorageBuffer:
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorType::CombinedImageSampler:
      return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorType::StorageImage:
      return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorType::InputAttachment:
      return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    default:
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
  }
};

class DescriptorSetLayout {
public:
  DescriptorSetLayout(VkDevice device,
                      const std::vector<DescriptorBinding> &bindings);
  DescriptorSetLayout(const VulkanContext &ctx,
                      const std::vector<DescriptorBinding> &bindings);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;
  DescriptorSetLayout(DescriptorSetLayout &&other) noexcept;
  DescriptorSetLayout &operator=(DescriptorSetLayout &&other) noexcept;

  [[nodiscard]] VkDescriptorSetLayout layout() const noexcept {
    return _layout;
  }

  [[nodiscard]] const std::vector<DescriptorBinding> &
  bindings() const noexcept {
    return _bindings;
  }

  static DescriptorSetLayout createDefault3D(VkDevice device);
  static DescriptorSetLayout createDefault3D(const VulkanContext &ctx);

private:
  VkDevice device{VK_NULL_HANDLE};
  VkDescriptorSetLayout _layout{VK_NULL_HANDLE};
  std::vector<DescriptorBinding> _bindings;

  void cleanup();
};

} // namespace vkr
