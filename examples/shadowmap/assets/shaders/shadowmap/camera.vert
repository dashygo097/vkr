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

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragViewPos;
layout(location = 3) out vec3 fragLightViewPos;
layout(location = 4) out vec4 fragLightClip;

void main() {
  vec4 worldPos = camera.model * vec4(inPosition, 1.0);
  vec4 viewPos = camera.view * worldPos;
  mat3 normalMatrix = transpose(inverse(mat3(camera.view * camera.model)));

  fragColor = inColor;
  fragNormal = normalize(normalMatrix * inNormal);
  fragViewPos = viewPos.xyz;
  fragLightViewPos = (camera.view * vec4(light.lightPosBias.xyz, 1.0)).xyz;
  fragLightClip = light.lightViewProj * worldPos;
  gl_Position = camera.proj * viewPos;
}
