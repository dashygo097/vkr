#version 450

layout(location = 0) in vec3 localPosition;
layout(location = 1) in vec3 vertexColor;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 color = vec3(0.78);

  if (localPosition.x < 0.08) {
    color = vec3(0.75, 0.12, 0.10);
  } else if (localPosition.x > 5.45) {
    color = vec3(0.10, 0.55, 0.18);
  } else if (localPosition.y > 5.40 && localPosition.x > 2.10 &&
             localPosition.x < 3.55 && localPosition.z > 2.05 &&
             localPosition.z < 3.70) {
    color = vec3(1.0, 0.92, 0.68);
  }

  float heightShade = clamp(localPosition.y / 5.49, 0.0, 1.0);
  float backShade = clamp(localPosition.z / 5.59, 0.0, 1.0);
  color *= 0.45 + 0.35 * heightShade + 0.20 * backShade;
  color *= max(vertexColor, vec3(0.75));

  outColor = vec4(color, 1.0);
}
