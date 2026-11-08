#pragma once

#include "vkr/pipeline/vertex_input.hh"
#include <cstddef>
#include <glm/glm.hpp>

namespace vkr::scene {

struct Vertex2D {
  glm::vec2 pos{};
  glm::vec3 color{};

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> pipe::VertexInputDesc {
    pipe::VertexInputDesc desc{};
    desc.binding<Vertex2D>(binding);
    desc.attribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, pos),
                   binding);
    desc.attribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex2D, color),
                   binding);
    return desc;
  }
};

struct Vertex3D {
  glm::vec3 pos{};
  glm::vec3 color{};

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> pipe::VertexInputDesc {
    pipe::VertexInputDesc desc{};
    desc.binding<Vertex3D>(binding);
    desc.attribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, pos),
                   binding);
    desc.attribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, color),
                   binding);
    return desc;
  }
};

struct VertexNormalTexture3D {
  glm::vec3 pos{};
  glm::vec3 color{};
  glm::vec3 normal{};
  glm::vec2 texCoord{};

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> pipe::VertexInputDesc {
    pipe::VertexInputDesc desc{};
    desc.binding<VertexNormalTexture3D>(binding);
    desc.attribute(0, VK_FORMAT_R32G32B32_SFLOAT,
                   offsetof(VertexNormalTexture3D, pos), binding);
    desc.attribute(1, VK_FORMAT_R32G32B32_SFLOAT,
                   offsetof(VertexNormalTexture3D, color), binding);
    desc.attribute(2, VK_FORMAT_R32G32B32_SFLOAT,
                   offsetof(VertexNormalTexture3D, normal), binding);
    desc.attribute(3, VK_FORMAT_R32G32_SFLOAT,
                   offsetof(VertexNormalTexture3D, texCoord), binding);
    return desc;
  }
};

struct VertexSkybox3D {
  glm::vec3 pos{};

  [[nodiscard]] static auto vertexInputDesc(uint32_t binding = 0)
      -> pipe::VertexInputDesc {
    pipe::VertexInputDesc desc{};
    desc.binding<VertexSkybox3D>(binding);
    desc.attribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSkybox3D, pos),
                   binding);
    return desc;
  }
};

} // namespace vkr::scene
