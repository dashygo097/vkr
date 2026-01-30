#pragma once

#include "../core/window.hh"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkr::scene {
class Camera {
public:
  explicit Camera(const core::Window &window, const float &movemeneSpeed,
                  const float &mouseSensitivity, const float &fov,
                  const float &aspectRatio, const float &nearPlane,
                  const float &farPlane);
  ~Camera() = default;

  Camera(const Camera &) = delete;
  Camera &operator=(const Camera &) = delete;

  void track();

  glm::vec3 pos() const { return pos_; }

  // update camera vectors
  glm::mat4 getView() const { return glm::lookAt(pos_, pos_ + front_, up_); }
  glm::mat4 getProjection() const {
    glm::mat4 proj = glm::perspective(glm::radians(fov_), aspect_ratio_,
                                      near_plane_, far_plane_);
    proj[1][1] *= -1;
    return proj;
  }

  void mouseMove(float xOffset, float yOffset) {
    yaw_ += xOffset * mouse_sensitivity_;
    pitch_ += yOffset * mouse_sensitivity_;
    if (pitch_ > 89.0f)
      pitch_ = 89.0f;
    if (pitch_ < -89.0f)
      pitch_ = -89.0f;

    glm::vec3 frontTemp;
    frontTemp.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    frontTemp.y = sin(glm::radians(pitch_));
    frontTemp.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));

    front_ = glm::normalize(frontTemp);
    right_ = glm::normalize(glm::cross(front_, world_up_));
    up_ = glm::normalize(glm::cross(right_, front_));
  }

  // controls
  void moveForward(float deltaTime) {
    pos_ += front_ * movement_speed_ * deltaTime;
  }
  void moveBackward(float deltaTime) {
    pos_ -= front_ * movement_speed_ * deltaTime;
  }
  void moveLeft(float deltaTime) {
    pos_ -= right_ * movement_speed_ * deltaTime;
  }
  void moveRight(float deltaTime) {
    pos_ += right_ * movement_speed_ * deltaTime;
  }
  void moveUp(float deltaTime) {
    pos_ += world_up_ * movement_speed_ * deltaTime;
  }
  void moveDown(float deltaTime) {
    pos_ -= world_up_ * movement_speed_ * deltaTime;
  }

  [[nodiscard]] bool isLocked() const noexcept { return locked_; }
  void lock(bool lock) { locked_ = lock; }

  void doLock() { locked_ = true; }
  void doUnlock() { locked_ = false; }
  void toggleLock() { locked_ = !locked_; }

private:
  // dependencies
  const core::Window &window_;
  const float &movement_speed_;
  const float &mouse_sensitivity_;
  const float &fov_;
  const float &aspect_ratio_;
  const float &near_plane_;
  const float &far_plane_;

  // components
  glm::vec3 pos_{glm::vec3(0.0f, 0.0f, 0.0f)};
  glm::vec3 front_{glm::vec3(0.0f, 0.0f, -1.0f)};
  glm::vec3 up_{glm::vec3(0.0f, 1.0f, 0.0f)};
  glm::vec3 right_{glm::vec3(1.0f, 0.0f, 0.0f)};
  glm::vec3 world_up_{glm::vec3(0.0f, 1.0f, 0.0f)};
  float yaw_{-90.0f};
  float pitch_{0.0f};

  // state
  bool locked_{false};
  std::chrono::high_resolution_clock::time_point last_time_stamp_;
  float first_mouse_{true};
  float last_x_{0.0f};
  float last_y_{0.0f};
};
} // namespace vkr::scene
