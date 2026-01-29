#include "vkr/scene/camera.hh"

namespace vkr::scene {
Camera::Camera(const core::Window &window, const float &movementSpeed,
               const float &mouseSensitivity, const float &fov,
               const float &aspectRatio, const float &nearPlane,
               const float &farPlane)
    : window(window), movementSpeed(movementSpeed),
      mouseSensitivity(mouseSensitivity), fov(fov), aspectRatio(aspectRatio),
      nearPlane(nearPlane), farPlane(farPlane), _pos(0.0f, 0.0f, 3.0f),
      _front(0.0f, 0.0f, -1.0f), _worldUp(0.0f, 1.0f, 0.0f), _yaw(-90.0f),
      _pitch(0.0f), _locked(false), lastX(window.width() / 2.0),
      lastY(window.height() / 2.0), firstMouse(true) {
  _right = glm::normalize(glm::cross(_front, _worldUp));
  _up = glm::normalize(glm::cross(_right, _front));
}

void Camera::track() {
  if (_locked)
    return;
  auto now = std::chrono::high_resolution_clock::now();
  if (lastTimeStamp.time_since_epoch().count() == 0) {
    lastTimeStamp = now;
    return;
  }
  auto deltaTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimeStamp)
          .count() /
      1000.0f;
  lastTimeStamp = now;

  if (glfwGetKey(window.glfwWindow(), GLFW_KEY_W) == GLFW_PRESS) {
    moveForward(deltaTime);
  }
  if (glfwGetKey(window.glfwWindow(), GLFW_KEY_S) == GLFW_PRESS) {
    moveBackward(deltaTime);
  }
  if (glfwGetKey(window.glfwWindow(), GLFW_KEY_A) == GLFW_PRESS) {
    moveLeft(deltaTime);
  }
  if (glfwGetKey(window.glfwWindow(), GLFW_KEY_D) == GLFW_PRESS) {
    moveRight(deltaTime);
  }
  if (glfwGetKey(window.glfwWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) {
    moveUp(deltaTime);
  }
  if (glfwGetKey(window.glfwWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    moveDown(deltaTime);
  }

  double xpos, ypos;
  glfwGetCursorPos(window.glfwWindow(), &xpos, &ypos);
  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
    return;
  }
  float xoffset = static_cast<float>(xpos - lastX);
  float yoffset = static_cast<float>(lastY - ypos);

  lastX = xpos;
  lastY = ypos;

  if (glfwGetMouseButton(window.glfwWindow(), GLFW_MOUSE_BUTTON_LEFT) ==
      GLFW_PRESS) {
    mouseMove(xoffset, yoffset);
  }
}

} // namespace vkr::scene
