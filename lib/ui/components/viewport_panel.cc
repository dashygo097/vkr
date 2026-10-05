#include "vkr/ui/components/viewport_panel.hh"
#include <imgui.h>

namespace vkr::ui {

namespace {

void drawViewportImage(VkDescriptorSet texture, const ImVec2 &size,
                       bool flipY) {
  const ImVec2 uv0 = flipY ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f);
  const ImVec2 uv1 = flipY ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f);
  ImGui::Image(reinterpret_cast<ImTextureID>(texture), size, uv0, uv1);
}

} // namespace

ViewportPanel::ViewportPanel(VkViewport &viewport, bool &focused, bool &hovered)
    : UiComponent("Viewport"), viewport_(viewport), focused_(focused),
      hovered_(hovered) {}

auto ViewportPanel::windowFlags() const noexcept -> ImGuiWindowFlags {
  return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
         ImGuiWindowFlags_NoScrollWithMouse;
}

void ViewportPanel::renderWindow() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  UiComponent::renderWindow();
  ImGui::PopStyleVar();
}

void ViewportPanel::render() {
  focused_ = false;
  hovered_ = false;
  viewport_ = {};
  const ImVec2 panelSize = ImGui::GetContentRegionAvail();
  if (panelSize.x < 1.0f || panelSize.y < 1.0f) {
    return;
  }

  if (texture_ == VK_NULL_HANDLE) {
    ImGui::TextDisabled("No render output");
    return;
  }

  const ImVec2 imagePosition = ImGui::GetCursorScreenPos();
  drawViewportImage(texture_, panelSize, flip_y_);

  viewport_.x = imagePosition.x;
  viewport_.y = imagePosition.y;
  viewport_.width = panelSize.x;
  viewport_.height = panelSize.y;
  viewport_.minDepth = 0.0f;
  viewport_.maxDepth = 1.0f;
  focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
  hovered_ = ImGui::IsItemHovered();
}

void ViewportPanel::renderFullscreen(VkDescriptorSet texture) {
  texture_ = texture;
  render();
}

} // namespace vkr::ui
