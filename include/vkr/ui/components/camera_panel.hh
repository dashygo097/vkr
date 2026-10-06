#pragma once

#include "vkr/scene/camera.hh"
#include "vkr/ui/components/ui_component.hh"
#include <vulkan/vulkan.h>

namespace vkr::ui {

class CameraPanel final : public UiComponent {
public:
  CameraPanel(scene::Camera &camera, const VkViewport &viewport,
              const bool &viewportFocused, const bool &viewportHovered);

private:
  void render() override;

  scene::Camera &camera_;
  const VkViewport &viewport_;
  const bool &viewport_focused_;
  const bool &viewport_hovered_;
};

} // namespace vkr::ui
