#pragma once

#include "../../core/device.hh"
#include <vector>

namespace vkr::pipeline {
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

  [[nodiscard]] auto toVkType() const noexcept -> VkDescriptorType {
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

  [[nodiscard]] auto toString() const noexcept -> std::string {
    std::string typeStr;
    switch (type) {
    case DescriptorType::UniformBuffer:
      typeStr = "uniform";
      break;
    case DescriptorType::StorageBuffer:
      typeStr = "storage";
      break;
    case DescriptorType::CombinedImageSampler:
      typeStr = "combined image sampler";
    case DescriptorType::StorageImage:
      typeStr = "storage image";
      break;
    case DescriptorType::InputAttachment:
      typeStr = "input attachment";
      break;
    }

    return "layout(binding=" + std::to_string(binding) + ", type=" + typeStr +
           ", count=" + std::to_string(count) +
           ", stageFlags=" + std::to_string(stageFlags) + ")";
  }
};

class DescriptorSetLayout {
public:
  explicit DescriptorSetLayout(const core::Device &device,
                               const std::vector<DescriptorBinding> &bindings);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  auto operator=(const DescriptorSetLayout &) -> DescriptorSetLayout & = delete;

  // FIXME: ref return
  [[nodiscard]] auto layoutRef() noexcept -> VkDescriptorSetLayout & { return layout_; }

  static auto createDefault3D(const core::Device &device) -> DescriptorSetLayout;

private:
  // dependencies
  const core::Device &device_;
  const std::vector<DescriptorBinding> &bindings_;

  // components
  VkDescriptorSetLayout layout_{VK_NULL_HANDLE};

  void cleanup();
};

} // namespace vkr::pipeline
