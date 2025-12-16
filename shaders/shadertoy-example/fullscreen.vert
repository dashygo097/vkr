#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec2 fragCoord;

void main() {
  gl_Position = vec4(inPosition, 1.0);
  fragCoord = (inPosition.xy + 1.0) * 0.5;
}
