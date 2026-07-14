#version 450

layout(binding = 1) uniform sampler2D albedoSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragViewPos;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 normal = normalize(fragNormal);
  vec3 viewDir = normalize(-fragViewPos);
  vec3 lightDir = normalize(vec3(0.35, 0.8, 0.45));

  float diffuse = max(dot(normal, lightDir), 0.0);
  float rim = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.0);

  vec3 textureColor = texture(albedoSampler, fragTexCoord).rgb;
  vec3 vertexTint = max(fragColor, vec3(0.35));
  vec3 base = textureColor * vertexTint;
  vec3 color = base * (0.30 + 0.85 * diffuse) + vec3(0.2, 0.55, 1.0) * rim;

  outColor = vec4(color, 1.0);
}
