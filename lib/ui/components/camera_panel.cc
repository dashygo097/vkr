#include "vkr/ui/components/camera_panel.hh"
#include "property_table.hh"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace vkr::ui {

CameraPanel::CameraPanel(scene::CameraDesc &camera, const VkViewport &viewport,
                         const bool &viewportFocused,
                         const bool &viewportHovered)
    : UiComponent("Camera"), camera_(camera), viewport_(viewport),
      viewport_focused_(viewportFocused), viewport_hovered_(viewportHovered) {}

void CameraPanel::render() {
  bool vectorsChanged = false;

  ImGui::SeparatorText("Transform");
  if (beginPropertyTable("##camera_transform")) {
    propertyRow("Position");
    if (ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 14.0f) {
      ImGui::DragFloat3("##position", glm::value_ptr(camera_.pos), 0.05f);
    } else {
      ImGui::DragFloat("##position_x", &camera_.pos.x, 0.05f, 0.0f, 0.0f,
                      "X: %.3f");
      ImGui::SetNextItemWidth(-1.0f);
      ImGui::DragFloat("##position_y", &camera_.pos.y, 0.05f, 0.0f, 0.0f,
                      "Y: %.3f");
      ImGui::SetNextItemWidth(-1.0f);
      ImGui::DragFloat("##position_z", &camera_.pos.z, 0.05f, 0.0f, 0.0f,
                      "Z: %.3f");
    }
    propertyRow("Yaw");
    vectorsChanged |= ImGui::SliderFloat("##yaw", &camera_.yaw, -180.0f, 180.0f,
                                        "%.1f deg");
    propertyRow("Pitch");
    vectorsChanged |= ImGui::SliderFloat("##pitch", &camera_.pitch, -89.0f, 89.0f,
                                        "%.1f deg");
    ImGui::EndTable();
  }

  if (vectorsChanged) {
    refreshCameraVectors(camera_);
  }

  ImGui::SeparatorText("Lens");
  if (beginPropertyTable("##camera_lens")) {
    propertyRow("FOV");
    ImGui::SliderFloat("##fov", &camera_.fov, 1.0f, 120.0f, "%.1f deg");
    propertyRow("Near plane");
    ImGui::DragFloat("##near_plane", &camera_.nearPlane, 0.01f, 0.001f,
                    camera_.farPlane - 0.001f, "%.3f");
    propertyRow("Far plane");
    ImGui::DragFloat("##far_plane", &camera_.farPlane, 1.0f,
                    camera_.nearPlane + 0.001f, 10000.0f, "%.1f");
    ImGui::EndTable();
  }

  if (camera_.farPlane <= camera_.nearPlane) {
    camera_.farPlane = camera_.nearPlane + 0.001f;
  }

  ImGui::SeparatorText("Input");
  if (beginPropertyTable("##camera_input")) {
    propertyRow("Locked");
    ImGui::Checkbox("##locked", &camera_.locked);
    propertyRow("Move speed");
    ImGui::DragFloat("##move_speed", &camera_.movementSpeed, 0.05f, 0.0f,
                    100.0f, "%.2f");
    propertyRow("Sensitivity");
    ImGui::DragFloat("##sensitivity", &camera_.mouseSensitivity, 0.01f, 0.0f,
                    10.0f, "%.2f");
    ImGui::EndTable();
  }

  ImGui::Spacing();
  if (ImGui::Button("Reset Camera")) {
    camera_.pos = glm::vec3{0.0f, 0.0f, 0.0f};
    camera_.yaw = -90.0f;
    camera_.pitch = 0.0f;
    camera_.fov = 45.0f;
    camera_.nearPlane = 0.1f;
    camera_.farPlane = 1000.0f;
    camera_.firstMouse = true;
    refreshCameraVectors(camera_);
  }

  ImGui::Spacing();
  if (ImGui::CollapsingHeader("Advanced")) {
    ImGui::SeparatorText("Orientation");
    if (beginPropertyTable("##camera_orientation")) {
      propertyRow("Front");
      ImGui::AlignTextToFramePadding();
      ImGui::TextWrapped("%.2f, %.2f, %.2f", camera_.front.x, camera_.front.y,
                         camera_.front.z);
      propertyRow("Up");
      ImGui::AlignTextToFramePadding();
      ImGui::TextWrapped("%.2f, %.2f, %.2f", camera_.up.x, camera_.up.y,
                         camera_.up.z);
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
}

void CameraPanel::refreshCameraVectors(scene::CameraDesc &camera) {
  camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

  glm::vec3 front{};
  front.x =
      std::cos(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch));
  front.y = std::sin(glm::radians(camera.pitch));
  front.z =
      std::sin(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch));

  camera.front = glm::normalize(front);
  camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
  camera.up = glm::normalize(glm::cross(camera.right, camera.front));
  camera.firstMouse = true;
}

} // namespace vkr::ui
