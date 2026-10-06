#pragma once

#include <glm/mat4x4.hpp>

namespace vkr::scene {

class Transform {
public:
  [[nodiscard]] auto matrix() const noexcept -> const glm::mat4 & {
    return matrix_;
  }

  void matrix(const glm::mat4 &value) noexcept { matrix_ = value; }

private:
  glm::mat4 matrix_{1.0f};
};

} // namespace vkr::scene
