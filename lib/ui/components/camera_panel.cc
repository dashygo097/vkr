#include "vkr/ui/components/camera_panel.hh"
#include "property_table.hh"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace vkr::ui {

CameraPanel::CameraPanel(scene::Camera &camera, const VkViewport &viewport,
                         const bool &viewportFocused,
                         const bool &viewportHovered)
    : UiComponent("Camera"), camera_(camera), viewport_(viewport),
      viewport_focused_(viewportFocused), viewport_hovered_(viewportHovered) {}

void CameraPanel::render() {
  auto camera = camera_.desc();
  bool changed = false;

  ImGui::SeparatorText("Transform");
  if (beginPropertyTable("##camera_transform")) {
    propertyRow("Position");
    if (ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 14.0f) {
      changed |= ImGui::DragFloat3("##position", glm::value_ptr(camera.pos), 0.05f);
    } else {
      changed |= ImGui::DragFloat("##position_x", &camera.pos.x, 0.05f, 0.0f, 0.0f,
                                  "X: %.3f");
      ImGui::SetNextItemWidth(-1.0f);
      changed |= ImGui::DragFloat("##position_y", &camera.pos.y, 0.05f, 0.0f, 0.0f,
                                  "Y: %.3f");
      ImGui::SetNextItemWidth(-1.0f);
      changed |= ImGui::DragFloat("##position_z", &camera.pos.z, 0.05f, 0.0f, 0.0f,
                                  "Z: %.3f");
    }
    propertyRow("Yaw");
    changed |= ImGui::SliderFloat("##yaw", &camera.yaw, -180.0f, 180.0f,
                                  "%.1f deg");
    propertyRow("Pitch");
    changed |= ImGui::SliderFloat("##pitch", &camera.pitch, -89.0f, 89.0f,
                                  "%.1f deg");
    ImGui::EndTable();
  }

  ImGui::SeparatorText("Lens");
  if (beginPropertyTable("##camera_lens")) {
    propertyRow("FOV");
    changed |= ImGui::SliderFloat("##fov", &camera.fov, 1.0f, 120.0f, "%.1f deg");
    propertyRow("Near plane");
    changed |= ImGui::DragFloat("##near_plane", &camera.nearPlane, 0.01f, 0.001f,
                                camera.farPlane - 0.001f, "%.3f");
    propertyRow("Far plane");
    changed |= ImGui::DragFloat("##far_plane", &camera.farPlane, 1.0f,
                                camera.nearPlane + 0.001f, 10000.0f, "%.1f");
    ImGui::EndTable();
  }

  if (camera.farPlane <= camera.nearPlane) {
    camera.farPlane = camera.nearPlane + 0.001f;
    changed = true;
  }

  ImGui::SeparatorText("Input");
  if (beginPropertyTable("##camera_input")) {
    propertyRow("Locked");
    changed |= ImGui::Checkbox("##locked", &camera.locked);
    propertyRow("Move speed");
    changed |= ImGui::DragFloat("##move_speed", &camera.movementSpeed, 0.05f, 0.0f,
                                100.0f, "%.2f");
    propertyRow("Sensitivity");
    changed |= ImGui::DragFloat("##sensitivity", &camera.mouseSensitivity,
                                0.01f, 0.0f, 10.0f, "%.2f");
    ImGui::EndTable();
  }

  ImGui::Spacing();
  if (ImGui::Button("Reset Camera")) {
    camera.pos = glm::vec3{0.0f, 0.0f, 0.0f};
    camera.yaw = -90.0f;
    camera.pitch = 0.0f;
    camera.fov = 45.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 1000.0f;
    camera.firstMouse = true;
    changed = true;
  }

  ImGui::Spacing();
  if (ImGui::CollapsingHeader("Advanced")) {
    ImGui::SeparatorText("Orientation");
    if (beginPropertyTable("##camera_orientation")) {
      propertyRow("Front");
      ImGui::AlignTextToFramePadding();
      ImGui::TextWrapped("%.2f, %.2f, %.2f", camera.front.x, camera.front.y,
                         camera.front.z);
      propertyRow("Up");
      ImGui::AlignTextToFramePadding();
      ImGui::TextWrapped("%.2f, %.2f, %.2f", camera.up.x, camera.up.y,
                         camera.up.z);
      ImGui::EndTable();
    }
    ImGui::SeparatorText("Viewport");
    if (beginPropertyTable("##camera_viewport")) {
      propertyRow("Position");
      ImGui::AlignTextToFramePadding();
      ImGui::TextWrapped("%.1f, %.1f", viewport_.x, viewport_.y);
      propertyRow("Size");
      ImGui::AlignTextToFramePadding();
      ImGui::TextWrapped("%.1f x %.1f", viewport_.width, viewport_.height);
      propertyRow("Focused", viewport_focused_ ? "Yes" : "No");
      propertyRow("Hovered", viewport_hovered_ ? "Yes" : "No");
      ImGui::EndTable();
    }
  }
  if (changed) {
    camera_.update(camera);
  }
}

} // namespace vkr::ui
