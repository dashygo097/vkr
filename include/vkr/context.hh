#pragma once

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
  uint32_t width{};
  uint32_t height{};
  std::string title{"Vulkan Application"};

  std::string appName{};
  uint32_t appVersion{};
  std::string assetsDir{DEFAULT_ASSETS_DIR};

  core::PresentModePolicy presentModePolicy{core::PresentModePolicy::Uncapped};
  pipeline::PipelineMode pipelineMode{pipeline::PipelineMode::Default3D};

  scene::CameraDesc camera{};

  uint32_t currentFrame{};
  bool framebufferResized{};

  std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
  std::vector<const char *> preExtensions = {
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
  };
  std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
  };

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return camera.isValid();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("camera", camera);
  }
};

} // namespace vkr
