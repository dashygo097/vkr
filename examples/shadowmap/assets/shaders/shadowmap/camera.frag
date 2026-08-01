#version 450

layout(binding = 1) uniform LightSpaceObject {
  mat4 lightViewProj;
  vec4 lightPosBias;
} light;

layout(binding = 2) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec4 fragLightClip;

layout(location = 0) out vec4 outColor;

float shadowVisibility(vec4 lightClip) {
  vec3 ndc = lightClip.xyz / lightClip.w;

  if (ndc.z < 0.0 || ndc.z > 1.0) {
    return 1.0;
  }

  vec2 uv = ndc.xy * 0.5 + 0.5;
  if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
    return 1.0;
  }

  float bias = light.lightPosBias.w;
  vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));

  float visibility = 0.0;
  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      float closestDepth = texture(shadowMap, uv + vec2(x, y) * texel).r;
      visibility += ndc.z - bias <= closestDepth ? 1.0 : 0.0;
    }
  }

  return visibility / 9.0;
}

void main() {
  vec3 normal = normalize(fragNormal);
  vec3 lightPos = light.lightPosBias.xyz;
  vec3 lightDir = normalize(lightPos - fragWorldPos);

  float diffuse = max(dot(normal, lightDir), 0.0);
  float visibility = shadowVisibility(fragLightClip);

  vec3 baseColor = max(fragColor, vec3(0.18));
  vec3 ambient = 0.18 * baseColor;
  vec3 direct = visibility * diffuse * baseColor;

  outColor = vec4(ambient + direct, 1.0);
}
