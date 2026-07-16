#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 sampleDirection;

void main() {
    sampleDirection = inPosition;

    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    vec4 position = ubo.proj * viewNoTranslation * vec4(inPosition, 1.0);
    gl_Position = position.xyww;
}
