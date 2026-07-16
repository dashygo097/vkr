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
  vec2 p = uv * 2.0 - 1.0;

  vec3 a = texture(iChannel1, uv).rgb;
  vec3 b = texture(iChannel2, uv).rgb;
  vec3 c = texture(iChannel3, uv).rgb;
  vec3 history = texture(iChannel0, uv).rgb;

  float vignette = smoothstep(1.2, 0.15, length(p));
  vec3 fresh = (a * 0.35 + b * 0.25 + c * 0.55) * vignette;
  fresh += 0.08 * vec3(sin(iTime + uv.x * 12.0),
                       sin(iTime * 1.3 + uv.y * 10.0),
                       sin(iTime * 0.7 + dot(uv, vec2(8.0))));

  float feedback = iFrame < 2 ? 0.0 : 0.9;
  color = vec4(mix(fresh, history * 0.975 + fresh * 0.08, feedback), 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * iResolution.xy;
  pixelCoord.y = iResolution.y - pixelCoord.y;
  mainImage(fragColor, pixelCoord);
}
