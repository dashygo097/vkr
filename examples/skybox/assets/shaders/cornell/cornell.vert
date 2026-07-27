#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 localPosition;
layout(location = 1) out vec3 vertexColor;

void main() {
  vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);

  gl_Position = ubo.proj * ubo.view * worldPosition;
  localPosition = inPosition;
  vertexColor = inColor;
}
