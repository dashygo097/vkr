#version 450

layout(binding = 0) uniform ShaderToyUBO {
  vec3 iResolution;
  float iTime;
  float iTimeDelta;
  float iFrameRate;
  int iFrame;
  vec4 iMouse;
  vec4 iDate;
  vec4 iChannelTime;
  vec3 iChannelResolution[4];
};

layout(binding = 1) uniform sampler2D iChannel0;
layout(binding = 2) uniform sampler2D iChannel1;

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

void mainImage(out vec4 color, in vec2 pixelCoord) {
  color = vec4(1.0);
}

void main() {
  mainImage(fragColor, fragCoord * iResolution.xy);
}
