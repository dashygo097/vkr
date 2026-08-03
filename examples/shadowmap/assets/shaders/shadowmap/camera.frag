#version 450

layout(binding = 1) uniform LightSpaceObject {
  mat4 lightViewProj;
  vec4 lightPosBias;
} light;

layout(binding = 2) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragViewPos;
layout(location = 3) in vec3 fragLightViewPos;
layout(location = 4) in vec4 fragLightClip;

layout(location = 0) out vec4 outColor;

float shadowVisibility(vec4 lightClip, vec3 normal, vec3 lightDir) {
  const int radius = 1;

  vec3 ndc = lightClip.xyz / lightClip.w;
  vec2 shadowUv = ndc.xy * 0.5 + 0.5;
  float currentDepth = ndc.z;

  bool outsideShadowMap =
      lightClip.w <= 0.0 || shadowUv.x < 0.0 || shadowUv.x > 1.0 ||
      shadowUv.y < 0.0 || shadowUv.y > 1.0 || currentDepth < 0.0 ||
      currentDepth > 1.0;

  if (outsideShadowMap) {
    return 1.0;
  }

  float baseBias = light.lightPosBias.w;
  float slopeBias = max(baseBias * (1.0 - dot(normal, lightDir)), baseBias);
  vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

  float visible = 0.0;
  int sampleCount = 0;
  for (int y = -radius; y <= radius; ++y) {
    for (int x = -radius; x <= radius; ++x) {
      vec2 offset = vec2(x, y) * texelSize;
      float shadowMapDepth = texture(shadowMap, shadowUv + offset).r;
      visible += currentDepth - slopeBias <= shadowMapDepth ? 1.0 : 0.0;
      sampleCount++;
    }
  }

  return visible / float(sampleCount);
}

vec3 blinnPhong(vec3 baseColor, vec3 normal, vec3 viewPos,
                vec3 lightViewPos, float visibility) {
  float ambientWeight = 0.08;
  float diffuseWeight = 0.78;
  float specularWeight = 0.28;
  float specularPower = 48.0;

  vec3 lightDir = normalize(lightViewPos - viewPos);
  vec3 viewDir = normalize(-viewPos);
  vec3 halfDir = normalize(lightDir + viewDir);

  float ndotl = max(dot(normal, lightDir), 0.0);
  float specularStrength =
      ndotl > 0.0 ? pow(max(dot(normal, halfDir), 0.0), specularPower) : 0.0;

  vec3 ambient = ambientWeight * baseColor;
  vec3 diffuse = diffuseWeight * ndotl * baseColor;
  vec3 specular = specularWeight * specularStrength * vec3(1.0);

  return ambient + visibility * (diffuse + specular);
}

void main() {
  vec3 baseColor = fragColor;
  vec3 normal = normalize(fragNormal);
  vec3 lightDir = normalize(fragLightViewPos - fragViewPos);
  float visibility = shadowVisibility(fragLightClip, normal, lightDir);
  vec3 color =
      blinnPhong(baseColor, normal, fragViewPos, fragLightViewPos, visibility);
  outColor = vec4(color, 1.0);
}
