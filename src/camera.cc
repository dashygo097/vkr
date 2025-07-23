#include "vkr/camera.hpp"

namespace vkr {

Camera::Camera(GLFWwindow *window) : window(window) {}
Camera::Camera(GLFWwindow *window, float movementSpeed, float mouseSensitivity,
               float aspectRatio, float fov, float nearPlane, float farPlane)
    : window(window), movementSpeed(movementSpeed),
      mouseSensitivity(mouseSensitivity), fov(fov), aspectRatio(aspectRatio),
      nearPlane(nearPlane), farPlane(farPlane) {}

Camera::Camera(const VulkanContext &ctx)
    : Camera(ctx.window, ctx.cameraMovementSpeed, ctx.cameraMouseSensitivity,
             ctx.cameraAspectRatio, ctx.cameraFov, ctx.cameraNearPlane,
             ctx.cameraFarPlane) {}

Camera::~Camera() {}

void Camera::track() {
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

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    moveForward(deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    moveBackward(deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    moveLeft(deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    moveRight(deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    moveUp(deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    moveDown(deltaTime);
  }

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
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

  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    mouseMove(xoffset, yoffset);
  }
}
} // namespace vkr
