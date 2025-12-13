#pragma once

#include "../ctx.hh"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

  glm::vec3 pos() const { return _pos; }

  // update camera vectors
  glm::mat4 getView() const { return glm::lookAt(_pos, _pos + _front, _up); }
  glm::mat4 getProjection() const {
    glm::mat4 proj = glm::perspective(glm::radians(_fov), _aspectRatio,
                                      _nearPlane, _farPlane);
    proj[1][1] *= -1;
    return proj;
  }

  void mouseMove(float xOffset, float yOffset) {
    _yaw += xOffset * _mouseSensitivity;
    _pitch += yOffset * _mouseSensitivity;
    if (_pitch > 89.0f)
      _pitch = 89.0f;
    if (_pitch < -89.0f)
      _pitch = -89.0f;

    glm::vec3 frontTemp;
    frontTemp.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    frontTemp.y = sin(glm::radians(_pitch));
    frontTemp.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));

    _front = glm::normalize(frontTemp);
    _right = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_right, _front));
  }

  // controls
  void moveForward(float deltaTime) {
    _pos += _front * _movementSpeed * deltaTime;
  }
  void moveBackward(float deltaTime) {
    _pos -= _front * _movementSpeed * deltaTime;
  }
  void moveLeft(float deltaTime) {
    _pos -= _right * _movementSpeed * deltaTime;
  }
  void moveRight(float deltaTime) {
    _pos += _right * _movementSpeed * deltaTime;
  }
  void moveUp(float deltaTime) {
    _pos += _worldUp * _movementSpeed * deltaTime;
  }
  void moveDown(float deltaTime) {
    _pos -= _worldUp * _movementSpeed * deltaTime;
  }

  float movementSpeed() const { return _movementSpeed; }
  void movementSpeed(float speed) { this->_movementSpeed = speed; }
  float mouseSensitivity() const { return _mouseSensitivity; }
  void mouseSensitivity(float sensitivity) {
    this->_mouseSensitivity = sensitivity;
  }

  [[nodiscard]] float fov() const noexcept { return _fov; }
  void fov(float fov) { this->_fov = fov; }
  [[nodiscard]] float aspectRatio() const noexcept { return _aspectRatio; }
  void aspectRatio(float aspectRatio) { this->_aspectRatio = aspectRatio; }
  [[nodiscard]] float nearPlane() const noexcept { return _nearPlane; }
  void nearPlane(float nearPlane) { this->_nearPlane = nearPlane; }
  [[nodiscard]] float farPlane() const noexcept { return _farPlane; }
  void farPlane(float farPlane) { this->_farPlane = farPlane; }
  [[nodiscard]] bool isLocked() const noexcept { return _locked; }
  void lock(bool lock) { _locked = lock; }

  void doLock() { _locked = true; }
  void doUnlock() { _locked = false; }
  void toggle_lock() { _locked = !_locked; }

private:
  // dependencies
  GLFWwindow *window{nullptr};

  // components
  float _movementSpeed;
  float _mouseSensitivity;
  float _fov;
  float _aspectRatio;
  float _nearPlane;
  float _farPlane;
  glm::vec3 _pos{glm::vec3(0.0f, 0.0f, 0.0f)};
  glm::vec3 _front{glm::vec3(0.0f, 0.0f, -1.0f)};
  glm::vec3 _up{glm::vec3(0.0f, 1.0f, 0.0f)};
  glm::vec3 _right{glm::vec3(1.0f, 0.0f, 0.0f)};
  glm::vec3 _worldUp{glm::vec3(0.0f, 1.0f, 0.0f)};
  float _yaw{-90.0f};
  float _pitch{0.0f};

  bool _locked{false};

  // timing
  std::chrono::high_resolution_clock::time_point lastTimeStamp;
  float firstMouse{true};
  float lastX{0.0f};
  float lastY{0.0f};
};
} // namespace vkr
