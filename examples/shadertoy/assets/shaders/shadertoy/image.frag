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
layout(binding = 3) uniform sampler2D iChannel2;
layout(binding = 4) uniform sampler2D iChannel3;

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;

  vec3 a = texture(iChannel0, uv).rgb;
  vec3 b = texture(iChannel1, uv).rgb;
  vec3 c = texture(iChannel2, uv).rgb;
  vec3 d = texture(iChannel3, uv).rgb;

  vec3 composite = a * 0.35 + b * 0.25 + c * 0.25 + d * 0.45;

  if (uv.x < 0.24 && uv.y > 0.76) {
    composite = a;
  } else if (uv.x < 0.49 && uv.y > 0.76) {
    composite = b;
  } else if (uv.x < 0.74 && uv.y > 0.76) {
    composite = c;
  } else if (uv.y > 0.76) {
    composite = d;
  }

  vec2 grid = abs(fract(uv * vec2(4.0, 4.0)) - 0.5);
  float line = 1.0 - smoothstep(0.0, 0.012, min(grid.x, grid.y));
  composite = mix(composite, vec3(1.0), line * 0.08);

  color = vec4(pow(max(composite, vec3(0.0)), vec3(0.9)), 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * iResolution.xy;
  pixelCoord.y = iResolution.y - pixelCoord.y;
  mainImage(fragColor, pixelCoord);
}
