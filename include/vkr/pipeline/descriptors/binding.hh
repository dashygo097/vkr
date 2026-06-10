#pragma once

#include "vkr/logger.hh"
#include <vulkan/vulkan.h>

namespace vkr::pipeline {

struct DescriptorBinding {
  std::string name;
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
           ", stageFlags=" + std::to_string(stageFlags) + ", resource=" + name +
           ")";
  }
};

} // namespace vkr::pipeline
