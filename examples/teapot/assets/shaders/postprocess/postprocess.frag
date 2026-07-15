#version 450

layout(binding = 0) uniform sampler2D sceneColor;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
  vec2 texel = 1.0 / vec2(textureSize(sceneColor, 0));
  float shift = 3.0;

  vec3 color;
  color.r = texture(sceneColor, fragUV + vec2(shift * texel.x, 0.0)).r;
  color.g = texture(sceneColor, fragUV).g;
  color.b = texture(sceneColor, fragUV - vec2(shift * texel.x, 0.0)).b;

  vec3 posterized = floor(color * 5.0) / 5.0;
  float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
  vec3 graded = mix(vec3(luma), posterized, 0.75);

  float scanline = 0.82 + 0.18 * sin(gl_FragCoord.y * 3.14159265);
  float vignette = smoothstep(0.85, 0.25, distance(fragUV, vec2(0.5)));
  vec3 tint = vec3(1.18, 0.92, 1.28);

  outColor = vec4(graded * tint * scanline * mix(0.45, 1.0, vignette), 1.0);
}
