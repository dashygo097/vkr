#version 450

layout(binding = 0) uniform sampler2D skyboxColor;
layout(binding = 1) uniform sampler2D cornellColor;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
  vec4 skybox = texture(skyboxColor, fragUV);
  vec4 cornell = texture(cornellColor, fragUV);

  outColor = vec4(mix(skybox.rgb, cornell.rgb, cornell.a), 1.0);
}
