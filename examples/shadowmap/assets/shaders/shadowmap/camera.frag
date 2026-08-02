#version 450

layout(binding = 1) uniform LightSpaceObject {
  mat4 lightViewProj;
  vec4 lightPosBias;
} light;

layout(binding = 2) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec4 fragLightClip;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 ndc = fragLightClip.xyz / fragLightClip.w;
  vec2 uv = ndc.xy * 0.5 + 0.5;
  float shadowDepth = texture(shadowMap, uv).r;
  outColor = vec4(fragColor, 1.0);
}
