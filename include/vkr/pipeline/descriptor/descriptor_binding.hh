#pragma once

#include "../../core/device.hh"
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
  explicit DescriptorSetLayout(const Device &device,
                               const std::vector<DescriptorBinding> &bindings);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

  // FIXME: ref return
  [[nodiscard]] VkDescriptorSetLayout &layout() noexcept { return _layout; }

  static DescriptorSetLayout createDefault3D(const Device &device);

private:
  // dependencies
  const Device &device;
  const std::vector<DescriptorBinding> &bindings;

  // components
  VkDescriptorSetLayout _layout{VK_NULL_HANDLE};

  void cleanup();
};

} // namespace vkr
