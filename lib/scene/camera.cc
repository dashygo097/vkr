#include "vkr/scene/camera.hh"
#include "GLFW/glfw3.h"

namespace vkr::scene {

Camera::Camera(const core::Window &window, const Timer &timer,
               pipeline::PipelineMode mode, CameraDesc &desc)
    : window_(window), timer_(timer), mode_(mode), desc_(desc) {
  desc_.lastX = static_cast<float>(window.width()) / 2.0f;
  desc_.lastY = static_cast<float>(window.height()) / 2.0f;

  updateVectors();

  glfwSetWindowUserPointer(window_.glfwWindow(), this);
  glfwSetScrollCallback(window_.glfwWindow(), scrollCallback);
}

void Camera::track() {
  if (desc_.locked) {
    desc_.firstMouse = true;
    return;
  }

  auto deltaTime = timer_.deltaTime();

  if (glfwGetKey(window_.glfwWindow(), GLFW_KEY_W) == GLFW_PRESS) {
    moveForward(deltaTime);
  }
  if (glfwGetKey(window_.glfwWindow(), GLFW_KEY_S) == GLFW_PRESS) {
    moveBackward(deltaTime);
  }
  if (glfwGetKey(window_.glfwWindow(), GLFW_KEY_A) == GLFW_PRESS) {
    moveLeft(deltaTime);
  }
  if (glfwGetKey(window_.glfwWindow(), GLFW_KEY_D) == GLFW_PRESS) {
    moveRight(deltaTime);
  }
  if (glfwGetKey(window_.glfwWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) {
    moveUp(deltaTime);
  }
  if (glfwGetKey(window_.glfwWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    moveDown(deltaTime);
  }

  double xpos, ypos;
  glfwGetCursorPos(window_.glfwWindow(), &xpos, &ypos);
  if (desc_.firstMouse) {
    desc_.lastX = xpos;
    desc_.lastY = ypos;
    desc_.firstMouse = false;
    return;
  }
  auto xoffset = static_cast<float>(xpos - desc_.lastX);
  auto yoffset = static_cast<float>(desc_.lastY - ypos);

  desc_.lastX = xpos;
  desc_.lastY = ypos;

  if (glfwGetMouseButton(window_.glfwWindow(), GLFW_MOUSE_BUTTON_LEFT) ==
      GLFW_PRESS) {
    mouseMove(xoffset, yoffset);
  }
}

} // namespace vkr::scene
