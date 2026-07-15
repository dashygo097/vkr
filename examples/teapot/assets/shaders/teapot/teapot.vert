#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragViewPos;
layout(location = 3) out vec2 fragTexCoord;

void main() {
  vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
  vec4 viewPos = ubo.view * worldPos;
  mat3 normalMatrix = transpose(inverse(mat3(ubo.view * ubo.model)));

  gl_Position = ubo.proj * viewPos;
  fragColor = inColor;
  fragNormal = normalize(normalMatrix * inNormal);
  fragViewPos = viewPos.xyz;
  fragTexCoord = inTexCoord;
}
