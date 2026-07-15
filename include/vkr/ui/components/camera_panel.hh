#pragma once

#include "vkr/scene/camera.hh"
#include "vkr/ui/components/ui_component.hh"
#include <vulkan/vulkan.h>

namespace vkr::ui {

class CameraPanel final : public UiComponent {
public:
  CameraPanel(scene::CameraDesc &camera, const VkViewport &viewport,
              const bool &viewportFocused, const bool &viewportHovered);

private:
  void render() override;
  static void refreshCameraVectors(scene::CameraDesc &camera);

  scene::CameraDesc &camera_;
  const VkViewport &viewport_;
  const bool &viewport_focused_;
  const bool &viewport_hovered_;
};

} // namespace vkr::ui
