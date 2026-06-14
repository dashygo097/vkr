#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/window.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/scene/camera.hh"
#include <GLFW/glfw3.h>
#include <string>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

namespace vkr {

struct VulkanContext {
  std::string assetsDir{DEFAULT_ASSETS_DIR};
  core::WindowDesc window{};
  core::InstanceDesc instance{};
  core::DeviceDesc device{};
  core::SwapchainDesc swapchain{};
  scene::CameraDesc camera{};

  pipeline::PipelineMode pipelineMode{pipeline::PipelineMode::Default3D};

  uint32_t currentFrame{};
  bool framebufferResized{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return window.isValid() && instance.isValid() && device.isValid() &&
           swapchain.isValid() && camera.isValid();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("window", window);
    ar("instance", instance);
    ar("device", device);
    ar("swapchain", swapchain);
    ar("camera", camera);
  }
};

} // namespace vkr
