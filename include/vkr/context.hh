#pragma once

#include "vkr/core/instance.hh"
#include "vkr/core/window.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/scene/camera.hh"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

namespace vkr {

struct VulkanContext {
  std::string assetsDir{DEFAULT_ASSETS_DIR};
  core::WindowDesc window{};
  core::InstanceDesc instance{};

  core::PresentModePolicy presentModePolicy{core::PresentModePolicy::Uncapped};
  pipeline::PipelineMode pipelineMode{pipeline::PipelineMode::Default3D};

  scene::CameraDesc camera{};

  uint32_t currentFrame{};
  bool framebufferResized{};

  std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
  };

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return window.isValid() && instance.isValid() && camera.isValid();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("window", window);
    ar("instance", instance);
    ar("camera", camera);
  }
};

} // namespace vkr
