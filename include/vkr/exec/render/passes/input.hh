#pragma once

#include <vulkan/vulkan.h>

namespace vkr::exec {

enum class RenderPassInputKind {
  Color,
  Depth,
};

struct RenderPassInputDesc {
  uint32_t binding{0};
  VkShaderStageFlags stageFlags{VK_SHADER_STAGE_FRAGMENT_BIT};
  RenderPassInputKind kind{RenderPassInputKind::Color};

  [[nodiscard]] static auto
  color(uint32_t binding,
        VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RenderPassInputDesc {
    return {.binding = binding,
            .stageFlags = stageFlags,
            .kind = RenderPassInputKind::Color};
  }

  [[nodiscard]] static auto
  depth(uint32_t binding,
        VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RenderPassInputDesc {
    return {.binding = binding,
            .stageFlags = stageFlags,
            .kind = RenderPassInputKind::Depth};
  }
};

} // namespace vkr::exec
