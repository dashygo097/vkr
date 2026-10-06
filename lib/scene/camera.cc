#include "vkr/scene/camera.hh"
#include "vkr/logger.hh"
#include <GLFW/glfw3.h>
#include <cmath>

namespace vkr::scene {

Camera::Camera(const util::Timer &timer, const util::InputTracer &input)
    : timer_(timer), input_(input) {}

Camera::~Camera() { destroy(); }

void Camera::create() {
  if (!desc_.isValid()) {
    VKR_SCENE_ERROR("Invalid camera descriptor");
  }
  destroy();
  desc_.pitch = glm::clamp(desc_.pitch, -89.0f, 89.0f);
  updateVectors();
  created_ = true;
}

void Camera::destroy() noexcept {
  created_ = false;
  desc_.firstMouse = true;
  desc_.lastX = 0.0f;
  desc_.lastY = 0.0f;
}

void Camera::update(const CameraDesc &desc) {
  if (!desc.isValid()) {
    VKR_SCENE_ERROR("Invalid camera descriptor");
  }
  desc_ = desc;
  create();
}

void Camera::aspect(float ratio) {
  if (!std::isfinite(ratio) || ratio <= 0.0f) {
    VKR_SCENE_ERROR("Camera aspect ratio must be finite and positive");
  }
  desc_.aspectRatio = ratio;
}

void Camera::track() {
  if (!created_) {
    VKR_SCENE_ERROR("Camera tracked before create");
  }
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
