#pragma once

#include "vkr/core/window.hh"
#include "vkr/util/input_tracer.hh"
#include "vkr/util/timer.hh"
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkr::scene {

struct CameraDesc {
  // config
  bool locked{false};
  float movementSpeed{0.0f};
  float mouseSensitivity{0.0f};
  float fov{45.0f};
  float aspectRatio{0.0f};
  float nearPlane{0.1f};
  float farPlane{1000.0f};

  // states
  glm::vec3 pos{0.0f, 0.0f, 0.0f};
  glm::vec3 front{0.0f, 0.0f, -1.0f};
  glm::vec3 up{0.0f, 1.0f, 0.0f};
  glm::vec3 right{1.0f, 0.0f, 0.0f};
  glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
  float yaw{-90.0f};
  float pitch{0.0f};

  // not serialized states
  bool firstMouse{true};
  float lastX{0.0f};
  float lastY{0.0f};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return nearPlane > 0.0f && farPlane > nearPlane;
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("locked", locked);
    ar("movementSpeed", movementSpeed);
    ar("mouseSensitivity", mouseSensitivity);
    ar("fov", fov);
    ar("aspectRatio", aspectRatio);
    ar("nearPlane", nearPlane);
    ar("farPlane", farPlane);

    ar("pos", pos);
    ar("front", front);
    ar("up", up);
    ar("right", right);
    ar("worldUp", worldUp);
    ar("yaw", yaw);
    ar("pitch", pitch);
  }
};

class Camera {
public:
  explicit Camera(const core::Window &window, const util::Timer &timer,
                  const util::InputTracer &input, CameraDesc &desc);
  ~Camera() = default;

  Camera(const Camera &) = delete;
  auto operator=(const Camera &) -> Camera & = delete;

  void track();

  [[nodiscard]] auto desc() const noexcept -> const CameraDesc & {
    return desc_;
  }

  [[nodiscard]] auto pos() const noexcept -> glm::vec3 { return desc_.pos; }
  [[nodiscard]] auto getView() const -> glm::mat4 {
    return glm::lookAt(desc_.pos, desc_.pos + desc_.front, desc_.up);
  }
  [[nodiscard]] auto getProjection() const -> glm::mat4 {
    glm::mat4 proj =
        glm::perspective(glm::radians(desc_.fov), desc_.aspectRatio,
                         desc_.nearPlane, desc_.farPlane);
    proj[1][1] *= -1;
    return proj;
  }

  void mouseMove(float xOffset, float yOffset) {
    desc_.yaw += xOffset * desc_.mouseSensitivity;
    desc_.pitch += yOffset * desc_.mouseSensitivity;

    if (desc_.pitch > 89.0f) {
      desc_.pitch = 89.0f;
    }
    if (desc_.pitch < -89.0f) {
      desc_.pitch = -89.0f;
    }

    updateVectors();
  };

  void moveForward(float deltaTime) {
    desc_.pos += desc_.front * desc_.movementSpeed * deltaTime;
  }

  void moveBackward(float deltaTime) {
    desc_.pos -= desc_.front * desc_.movementSpeed * deltaTime;
  }

  void moveLeft(float deltaTime) {
    desc_.pos -= desc_.right * desc_.movementSpeed * deltaTime;
  }

  void moveRight(float deltaTime) {
    desc_.pos += desc_.right * desc_.movementSpeed * deltaTime;
  }

  void moveUp(float deltaTime) {
    desc_.pos += desc_.worldUp * desc_.movementSpeed * deltaTime;
  }

  void moveDown(float deltaTime) {
    desc_.pos -= desc_.worldUp * desc_.movementSpeed * deltaTime;
  }

  void toggleLock() noexcept { lock(!desc_.locked); }

  void lock(bool locked) noexcept { desc_.locked = locked; }

  [[nodiscard]] auto isLocked() const noexcept -> bool { return desc_.locked; }

private:
  // dependencies
  const util::Timer &timer_;
  const util::InputTracer &input_;

  // components
  CameraDesc &desc_;

  // helpers
  void updateVectors() {
    glm::vec3 frontTemp;
    frontTemp.x =
        std::cos(glm::radians(desc_.yaw)) * std::cos(glm::radians(desc_.pitch));
    frontTemp.y = std::sin(glm::radians(desc_.pitch));
    frontTemp.z =
        std::sin(glm::radians(desc_.yaw)) * std::cos(glm::radians(desc_.pitch));

    desc_.front = glm::normalize(frontTemp);
    desc_.right = glm::normalize(glm::cross(desc_.front, desc_.worldUp));
    desc_.up = glm::normalize(glm::cross(desc_.right, desc_.front));
  };

  void updateZoom(float scrollOffset);
};

} // namespace vkr::scene
