#include "vkr/ui/components/camera_panel.hh"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace vkr::ui {

CameraPanel::CameraPanel(scene::CameraDesc &camera,
                         const VkViewport &viewport,
                         const bool &viewportFocused,
                         const bool &viewportHovered)
    : UiComponent("Camera"), camera_(camera), viewport_(viewport),
      viewport_focused_(viewportFocused), viewport_hovered_(viewportHovered) {}

void CameraPanel::render() {
  bool vectorsChanged = false;

  ImGui::SeparatorText("Transform");
  ImGui::DragFloat3("Position", glm::value_ptr(camera_.pos), 0.05f);

  vectorsChanged |=
      ImGui::SliderFloat("Yaw", &camera_.yaw, -180.0f, 180.0f, "%.1f deg");
  vectorsChanged |=
      ImGui::SliderFloat("Pitch", &camera_.pitch, -89.0f, 89.0f, "%.1f deg");

  if (vectorsChanged) {
    refreshCameraVectors(camera_);
  }

  ImGui::Text("Front: %.2f, %.2f, %.2f", camera_.front.x, camera_.front.y,
              camera_.front.z);
  ImGui::Text("Up: %.2f, %.2f, %.2f", camera_.up.x, camera_.up.y,
              camera_.up.z);

  ImGui::SeparatorText("Lens");
  ImGui::SliderFloat("FOV", &camera_.fov, 1.0f, 120.0f, "%.1f deg");
  ImGui::DragFloat("Near Plane", &camera_.nearPlane, 0.01f, 0.001f,
                   camera_.farPlane - 0.001f, "%.3f");
  ImGui::DragFloat("Far Plane", &camera_.farPlane, 1.0f,
                   camera_.nearPlane + 0.001f, 10000.0f, "%.1f");

  if (camera_.farPlane <= camera_.nearPlane) {
    camera_.farPlane = camera_.nearPlane + 0.001f;
  }

  ImGui::SeparatorText("Input");
  ImGui::Checkbox("Locked", &camera_.locked);
  ImGui::DragFloat("Move Speed", &camera_.movementSpeed, 0.05f, 0.0f, 100.0f,
                   "%.2f");
  ImGui::DragFloat("Mouse Sensitivity", &camera_.mouseSensitivity, 0.01f, 0.0f,
                   10.0f, "%.2f");

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

  ImGui::SeparatorText("Viewport");
  ImGui::Text("Position: %.1f, %.1f", viewport_.x, viewport_.y);
  ImGui::Text("Size: %.1f x %.1f", viewport_.width, viewport_.height);
  ImGui::Text("Focused: %s", viewport_focused_ ? "yes" : "no");
  ImGui::Text("Hovered: %s", viewport_hovered_ ? "yes" : "no");
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
