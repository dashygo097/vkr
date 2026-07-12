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

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  vec2 uv = fragCoord / iResolution.xy;
  uv = uv * 2.0 - 1.0;
  uv.x *= iResolution.x / iResolution.y;

  vec3 color = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));

  fragColor = vec4(color, 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * iResolution.xy;
  pixelCoord.y = iResolution.y - pixelCoord.y;
  mainImage(fragColor, pixelCoord);
}
