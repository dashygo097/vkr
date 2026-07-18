#include "vkr/scene/camera.hh"
#include <GLFW/glfw3.h>

namespace vkr::scene {

Camera::Camera(const util::Timer &timer, const util::InputTracer &input,
               CameraDesc &desc)
    : timer_(timer), input_(input), desc_(desc) {
  updateVectors();
}

void Camera::track() {
  if (desc_.locked) {
    desc_.firstMouse = true;
    return;
  }

  auto deltaTime = timer_.deltaTime();

  if (input_.isKeyDown(GLFW_KEY_W)) {
    moveForward(deltaTime);
  }
  if (input_.isKeyDown(GLFW_KEY_S)) {
    moveBackward(deltaTime);
  }
  if (input_.isKeyDown(GLFW_KEY_A)) {
    moveLeft(deltaTime);
  }
  if (input_.isKeyDown(GLFW_KEY_D)) {
    moveRight(deltaTime);
  }
  if (input_.isKeyDown(GLFW_KEY_SPACE)) {
    moveUp(deltaTime);
  }
  if (input_.isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
    moveDown(deltaTime);
  }

  updateZoom(static_cast<float>(input_.scrollOffset().y));

  const auto cursor = input_.cursorPosition();
  if (desc_.firstMouse) {
    desc_.lastX = static_cast<float>(cursor.x);
    desc_.lastY = static_cast<float>(cursor.y);
    desc_.firstMouse = false;
    return;
  }
  auto xoffset = static_cast<float>(cursor.x - desc_.lastX);
  auto yoffset = static_cast<float>(desc_.lastY - cursor.y);

  desc_.lastX = static_cast<float>(cursor.x);
  desc_.lastY = static_cast<float>(cursor.y);

  if (input_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
    mouseMove(xoffset, yoffset);
  }
}

void Camera::updateZoom(float scrollOffset) {
  if (scrollOffset == 0.0f) {
    return;
  }

  constexpr float kMinFov = 1.0f;
  constexpr float kMaxFov = 120.0f;
  constexpr float kZoomSpeed = 2.0f;

  desc_.fov -= scrollOffset * kZoomSpeed;
  desc_.fov = glm::clamp(desc_.fov, kMinFov, kMaxFov);
}

} // namespace vkr::scene
