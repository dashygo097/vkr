#pragma once

#include <glm/glm.hpp>

namespace vkr::resource {

// uniform buffer for 3d object
struct UniformBuffer3DObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

// shader toy uniform buffer
struct UniformBufferShaderToyObject {
  alignas(16) glm::vec3 iResolution;
  alignas(4) float iTime;
  alignas(4) float iTimeDelta;
  alignas(4) float iFrameRate;
  alignas(4) int iFrame;
  alignas(16) glm::vec4 iMouse;
  alignas(16) glm::vec4 iDate;
  alignas(16) glm::vec4 iChannelTime;
  alignas(16) glm::vec3 iChannelResolution[4];
};

} // namespace vkr::resource
