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

  vec2 centered = uv - 0.5;

  centered.x *= ubo.iResolution.x / ubo.iResolution.y;

  float angle = ubo.iTime;
  vec2 rotated = vec2(
      centered.x * cos(angle) - centered.y * sin(angle),
      centered.x * sin(angle) + centered.y * cos(angle)
    );

  float dist = length(rotated);
  float circle = smoothstep(0.3, 0.29, dist);

  vec3 color = 0.5 + 0.5 * cos(ubo.iTime + uv.xyx + vec3(0, 2, 4));

  fragColor = vec4(color * circle, 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * ubo.iResolution.xy;

  mainImage(fragColor, pixelCoord);
}
