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

ViewportPanel::ViewportPanel(VkViewport &viewport, bool &focused,
                             bool &hovered)
    : UiComponent("Viewport"), viewport_(viewport), focused_(focused),
      hovered_(hovered) {}

auto ViewportPanel::windowFlags() const noexcept -> ImGuiWindowFlags {
  return ImGuiWindowFlags_NoCollapse;
}

void ViewportPanel::renderWindow() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  UiComponent::renderWindow();
  ImGui::PopStyleVar();
}

void ViewportPanel::render() {
  ImVec2 panelSize = ImGui::GetContentRegionAvail();

  if (panelSize.x < 1.0f) {
    panelSize.x = 1.0f;
  }

  if (panelSize.y < 1.0f) {
    panelSize.y = 1.0f;
  }

  if (texture_ != VK_NULL_HANDLE) {
    drawViewportImage(texture_, panelSize, flip_y_);
  } else {
    ImGui::TextDisabled("(no offscreen target)");
  }

  ImVec2 windowPos = ImGui::GetWindowPos();
  ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
  ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

  viewport_.x = windowPos.x + contentMin.x;
  viewport_.y = windowPos.y + contentMin.y;
  viewport_.width = contentMax.x - contentMin.x;
  viewport_.height = contentMax.y - contentMin.y;
  viewport_.minDepth = 0.0f;
  viewport_.maxDepth = 1.0f;
  focused_ =
      ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
  hovered_ = ImGui::IsWindowHovered();
}

void ViewportPanel::renderFullscreen(VkDescriptorSet texture) {
  ImVec2 panelSize = ImGui::GetContentRegionAvail();

  if (texture != VK_NULL_HANDLE) {
    drawViewportImage(texture, panelSize, flip_y_);
  }

  ImVec2 windowPos = ImGui::GetWindowPos();
  ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
  ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

  viewport_.x = windowPos.x + contentMin.x;
  viewport_.y = windowPos.y + contentMin.y;
  viewport_.width = contentMax.x - contentMin.x;
  viewport_.height = contentMax.y - contentMin.y;
  viewport_.minDepth = 0.0f;
  viewport_.maxDepth = 1.0f;
  focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
  hovered_ = ImGui::IsWindowHovered();
}

} // namespace vkr::ui
