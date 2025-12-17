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
} ubo;

layout(location = 0) in vec2 fragCoord;

layout(location = 0) out vec4 fragColor;

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  vec2 uv = fragCoord / ubo.iResolution.xy;
  uv = uv * 2.0 - 1.0;
  uv.x *= ubo.iResolution.x / ubo.iResolution.y;

  float dist = length(uv);

  float wave = sin(dist * 10.0 - ubo.iTime * 2.0) * 0.5 + 0.5;

  vec3 color = 0.5 + 0.5 * cos(ubo.iTime + uv.xyx + vec3(0, 2, 4));

  color *= wave;

  fragColor = vec4(color, 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * ubo.iResolution.xy;
  mainImage(fragColor, pixelCoord);
}
