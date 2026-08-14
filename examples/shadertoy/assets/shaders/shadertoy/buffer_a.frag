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

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void mainImage(out vec4 color, in vec2 pixelCoord) {
  color = vec4(0.0, 0.0, 0.0, 1.0);
}

void main() {
  mainImage(outColor, fragUV * iResolution.xy);
}
