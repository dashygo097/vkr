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

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  vec2 p = uv * 2.0 - 1.0;
  p.x *= iResolution.x / iResolution.y;

  vec2 mouse = iMouse.xy / max(iResolution.xy, vec2(1.0));
  float mousePulse = iMouse.z > 0.0 ? 0.2 : 0.0;
  float wave = sin(8.0 * length(p) - iTime * 3.0);
  vec3 fresh = 0.5 + 0.5 * cos(iTime + wave + vec3(0.0, 2.0, 4.0));
  fresh += mousePulse * vec3(mouse, 1.0 - mouse.x);

  vec3 history = texture(iChannel0, uv).rgb;
  float feedback = iFrame < 2 ? 0.0 : 0.92;
  color = vec4(mix(fresh, history * 0.985 + fresh * 0.08, feedback), 1.0);
}

void main() {
  vec2 pixelCoord = fragCoord * iResolution.xy;
  pixelCoord.y = iResolution.y - pixelCoord.y;
  mainImage(fragColor, pixelCoord);
}
