#pragma once

#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include <vector>

namespace vkr::pipeline {

struct DescriptorBinding {
  uint32_t binding;
  VkDescriptorType type;
  uint32_t count{1};
  VkShaderStageFlags stageFlags;

  [[nodiscard]] auto toString() const noexcept -> std::string {
    std::string typeStr;
    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      typeStr = "uniform";
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      typeStr = "storage";
      break;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      typeStr = "combined image sampler";
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      typeStr = "storage image";
      break;
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      typeStr = "input attachment";
      break;
    default:
      VKR_PIPE_ERROR("Unknown descriptor type: {}", std::to_string(type));
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

  [[nodiscard]] auto layoutRef() noexcept -> VkDescriptorSetLayout & {
    return layout_;
  }

private:
  // dependencies
  const core::Device &device_;
  const std::vector<DescriptorBinding> &bindings_;

  // components
  VkDescriptorSetLayout layout_{VK_NULL_HANDLE};
};

} // namespace vkr::pipeline
