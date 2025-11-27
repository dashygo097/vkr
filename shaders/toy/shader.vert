#version 450

layout(location = 0) out vec2 fragCoord;

void main() {
    vec2 uv = vec2((gl_VertexIndex & 1), (gl_VertexIndex >> 1) & 1);
    gl_Position = vec4(uv * 4.0 - 2.0, 0.0, 1.0);
    fragCoord = uv;
}
