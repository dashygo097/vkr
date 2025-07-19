#pragma once

#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "./ctx.hpp"

namespace vkr {
class Camera {
public:
  Camera(GLFWwindow *window);
  Camera(GLFWwindow *window, float movementSpeed, float mouseSensitivity,
         float fov, float aspectRatio, float nearPlane, float farPlane);
  Camera(const VulkanContext &ctx);
  ~Camera();

  Camera(const Camera &) = delete;
  Camera &operator=(const Camera &) = delete;

  void track();

  glm::vec3 getPos() const { return pos; }

  // update camera vectors
  glm::mat4 getView() const { return glm::lookAt(pos, pos + front, up); }
  glm::mat4 getProjection() const {
    glm::mat4 proj =
        glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    proj[1][1] *= -1;
    return proj;
  }

  void mouseMove(float xOffset, float yOffset) {
    yaw += xOffset * mouseSensitivity;
    pitch += yOffset * mouseSensitivity;
    if (pitch > 89.0f)
      pitch = 89.0f;
    if (pitch < -89.0f)
      pitch = -89.0f;

    glm::vec3 frontTemp;
    frontTemp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    frontTemp.y = sin(glm::radians(pitch));
    frontTemp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    front = glm::normalize(frontTemp);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
  }

  // controls
  void moveForward(float deltaTime) {
    pos += front * movementSpeed * deltaTime;
  }
  void moveBackward(float deltaTime) {
    pos -= front * movementSpeed * deltaTime;
  }
  void moveLeft(float deltaTime) { pos -= right * movementSpeed * deltaTime; }
  void moveRight(float deltaTime) { pos += right * movementSpeed * deltaTime; }
  void moveUp(float deltaTime) { pos += worldUp * movementSpeed * deltaTime; }
  void moveDown(float deltaTime) { pos -= worldUp * movementSpeed * deltaTime; }

  float getMovementSpeed() const { return movementSpeed; }
  void setMovementSpeed(float speed) { this->movementSpeed = speed; }
  float getMouseSensitivity() const { return mouseSensitivity; }
  void setMouseSensitivity(float sensitivity) {
    this->mouseSensitivity = sensitivity;
  }
  float getFov() const { return fov; }
  void setFov(float fov) { this->fov = fov; }
  float getAspectRatio() const { return aspectRatio; }
  void setAspectRatio(float aspectRatio) { this->aspectRatio = aspectRatio; }
  float getNearPlane() const { return nearPlane; }
  void setNearPlane(float nearPlane) { this->nearPlane = nearPlane; }
  float getFarPlane() const { return farPlane; }
  void setFarPlane(float farPlane) { this->farPlane = farPlane; }

private:
  // dependencies
  GLFWwindow *window{nullptr};
  float movementSpeed;
  float mouseSensitivity;
  float fov;
  float aspectRatio;
  float nearPlane;
  float farPlane;

  // components
  glm::vec3 pos{glm::vec3(0.0f, 0.0f, 0.0f)};
  glm::vec3 front{glm::vec3(0.0f, 0.0f, -1.0f)};
  glm::vec3 up{glm::vec3(0.0f, 1.0f, 0.0f)};
  glm::vec3 right{glm::vec3(1.0f, 0.0f, 0.0f)};
  glm::vec3 worldUp{glm::vec3(0.0f, 1.0f, 0.0f)};
  float yaw{-90.0f};
  float pitch{0.0f};

  // timing
  std::chrono::high_resolution_clock::time_point lastTimeStamp;
  float firstMouse{true};
  float lastX{0.0f};
  float lastY{0.0f};
};
} // namespace vkr
