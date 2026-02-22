#include "vkr/scene/camera.hh"
#include "GLFW/glfw3.h"

namespace vkr::scene {
Camera::Camera(const core::Window &window, const Timer &timer,
               const pipeline::PipelineMode mode, const float &movementSpeed,
               const float &mouseSensitivity, const float &fov,
               const float &aspectRatio, const float &nearPlane,
               const float &farPlane)
    : window_(window), timer_(timer), mode_(mode),
      movement_speed_(movementSpeed), mouse_sensitivity_(mouseSensitivity),
      fov_(fov), aspect_ratio_(aspectRatio), near_plane_(nearPlane),
      far_plane_(farPlane), locked_(false), last_x_(window.width() / 2.0),
      last_y_(window.height() / 2.0), first_mouse_(true) {
  right_ = glm::normalize(glm::cross(front_, world_up_));
  up_ = glm::normalize(glm::cross(right_, front_));

  switch (mode_) {
  case pipeline::PipelineMode::Default3D:
  case pipeline::PipelineMode::Textured3D: {
    glfwSetWindowUserPointer(window_.glfwWindow(), this);
    glfwSetScrollCallback(window_.glfwWindow(), scrollCallback);
    break;
  }
  case pipeline::PipelineMode::Default2D:
  case pipeline::PipelineMode::Textured2D:
  case pipeline::PipelineMode::NoVertices:
  default: {
    break;
  }
  }
}

void Camera::track() {
  if (locked_)
    return;

  auto deltaTime = timer_.deltaTime();

  switch (mode_) {
  case pipeline::PipelineMode::Default3D:
  case pipeline::PipelineMode::Textured3D: {
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
    if (first_mouse_) {
      last_x_ = xpos;
      last_y_ = ypos;
      first_mouse_ = false;
      return;
    }
    float xoffset = static_cast<float>(xpos - last_x_);
    float yoffset = static_cast<float>(last_y_ - ypos);

    last_x_ = xpos;
    last_y_ = ypos;

    if (glfwGetMouseButton(window_.glfwWindow(), GLFW_MOUSE_BUTTON_LEFT) ==
        GLFW_PRESS) {
      mouseMove(xoffset, yoffset);
    }

    // mouse scroll to control fov

    break;
  }

  case pipeline::PipelineMode::Default2D:
  case pipeline::PipelineMode::Textured2D:
  case pipeline::PipelineMode::NoVertices:
  default: {
    break;
  }
  }
}

} // namespace vkr::scene
