#pragma once

#include "vkr/logger.hh"
#include <vulkan/vulkan.h>

namespace vkr::pipeline {

struct DescriptorBinding {
  std::string name{};
  VkDescriptorSetLayoutBinding layout{};

  [[nodiscard]] auto toString() const -> std::string {
    std::string typeStr{};

    switch (layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_SAMPLER:
      typeStr = "sampler";
      break;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      typeStr = "combined image sampler";
      break;
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      typeStr = "sampled image";
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      typeStr = "storage image";
      break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      typeStr = "uniform texel buffer";
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      typeStr = "storage texel buffer";
      break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      typeStr = "uniform buffer";
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      typeStr = "storage buffer";
      break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      typeStr = "uniform buffer dynamic";
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      typeStr = "storage buffer dynamic";
      break;
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      typeStr = "input attachment";
      break;
    default:
      VKR_PIPE_ERROR("Unknown descriptor type: {}",
                     static_cast<int>(layout.descriptorType));
    }

    return "layout(binding=" + std::to_string(layout.binding) +
           ", type=" + typeStr +
           ", count=" + std::to_string(layout.descriptorCount) +
           ", stageFlags=" + std::to_string(layout.stageFlags) +
           ", resource=" + name + ")";
  }
};

} // namespace vkr::pipeline
