#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

void main() {
  vec2 uv = fragCoord;
  fragColor = vec4(uv.x, uv.y, 0.0, 1.0);
}
