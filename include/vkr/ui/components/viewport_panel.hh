#pragma once

#include "vkr/ui/components/ui_component.hh"
#include <vulkan/vulkan.h>

namespace vkr::ui {

class ViewportPanel final : public UiComponent {
public:
  ViewportPanel(VkViewport &viewport, bool &focused, bool &hovered);

  void setTexture(VkDescriptorSet texture) noexcept { texture_ = texture; }
  void renderFullscreen(VkDescriptorSet texture);

  void renderWindow() override;

private:
  void render() override;
  [[nodiscard]] auto windowFlags() const noexcept -> ImGuiWindowFlags override;

  VkViewport &viewport_;
  bool &focused_;
  bool &hovered_;
  VkDescriptorSet texture_{VK_NULL_HANDLE};
};

} // namespace vkr::ui
