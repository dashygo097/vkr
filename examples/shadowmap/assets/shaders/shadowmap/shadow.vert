#version 450

layout(binding = 0) uniform CameraObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} camera;

layout(binding = 1) uniform LightSpaceObject {
  mat4 lightViewProj;
  vec4 lightPosBias;
} light;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

void main() {
  vec3 position = inPosition + (inColor + inNormal) * 0.0;
  gl_Position = light.lightViewProj * camera.model * vec4(position, 1.0);
}
