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

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

float luma(vec3 c) {
  return dot(c, vec3(0.299, 0.587, 0.114));
}

void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  vec2 texel = 1.0 / iResolution.xy;

  vec3 a = texture(iChannel1, uv).rgb;
  vec3 b = texture(iChannel2, uv).rgb;
  float edge = abs(luma(texture(iChannel2, uv + vec2(texel.x, 0.0)).rgb) -
                   luma(texture(iChannel2, uv - vec2(texel.x, 0.0)).rgb));
  edge += abs(luma(texture(iChannel2, uv + vec2(0.0, texel.y)).rgb) -
              luma(texture(iChannel2, uv - vec2(0.0, texel.y)).rgb));

  vec3 history = texture(iChannel0, uv).rgb;
  vec3 fresh = mix(a, b, 0.5) + edge * vec3(1.1, 0.7, 0.25);
  float feedback = iFrame < 2 ? 0.0 : 0.82;
  color = vec4(mix(fresh, history * 0.94 + fresh * 0.16, feedback), 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * iResolution.xy;
  pixelCoord.y = iResolution.y - pixelCoord.y;
  mainImage(fragColor, pixelCoord);
}
