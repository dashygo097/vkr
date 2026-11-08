#pragma once

#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

namespace vkr::util {

struct ObjVertex {
  glm::vec3 pos{};
  glm::vec3 color{1.0f};
  glm::vec3 normal{};
  glm::vec2 texCoord{};
};

struct ObjMesh {
  std::vector<ObjVertex> vertices{};
  std::vector<uint16_t> indices{};
};

[[nodiscard]] auto loadObj(const std::filesystem::path &path) -> ObjMesh;

} // namespace vkr::util
