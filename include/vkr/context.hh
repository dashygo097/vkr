#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/window.hh"
#include "vkr/scene/camera.hh"
#include "vkr/ui/theme.hh"
#include "vkr/util/asset.hh"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

namespace vkr {

struct VulkanContext {
  util::AssetDesc asset{};
  core::WindowDesc window{};
  core::InstanceDesc instance{};
  core::DeviceDesc device{};
  core::SwapchainDesc swapchain{};
  scene::CameraDesc camera{};
  ui::ThemeDesc theme{};

  uint32_t currentFrame{};
  bool framebufferResized{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return asset.isValid() && window.isValid() && instance.isValid() &&
           device.isValid() && swapchain.isValid() && camera.isValid() &&
           theme.isValid();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("asset", asset);
    ar("window", window);
    ar("instance", instance);
    ar("device", device);
    ar("swapchain", swapchain);
    ar("camera", camera);
    ar("theme", theme);
  }
};

} // namespace vkr
