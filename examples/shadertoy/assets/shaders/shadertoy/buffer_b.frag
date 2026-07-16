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
  vec2 uv = pixelCoord / iResolution.xy;
  vec2 texel = 1.0 / iResolution.xy;

  vec3 a = texture(iChannel1, uv).rgb;
  vec3 blur = texture(iChannel1, uv + vec2(texel.x, 0.0)).rgb;
  blur += texture(iChannel1, uv - vec2(texel.x, 0.0)).rgb;
  blur += texture(iChannel1, uv + vec2(0.0, texel.y)).rgb;
  blur += texture(iChannel1, uv - vec2(0.0, texel.y)).rgb;
  blur = (blur + a * 4.0) / 8.0;

  vec3 history = texture(iChannel0, uv).rgb;
  float feedback = iFrame < 2 ? 0.0 : 0.86;
  vec3 shifted = vec3(blur.r, a.g, blur.b);
  color = vec4(mix(shifted, history * 0.96 + shifted * 0.12, feedback), 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * iResolution.xy;
  pixelCoord.y = iResolution.y - pixelCoord.y;
  mainImage(fragColor, pixelCoord);
}
