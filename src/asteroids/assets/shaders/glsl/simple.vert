#version 450

layout(location=0) in vec4 position;
layout(location=1) in vec4 color;

layout(binding = 0) uniform Uniforms {
    mat4 projection;
    mat4 view;
    mat4 world;
} uniforms;

layout(location=0) out vec4 int_color;

void main() {
    vec4 out_position = uniforms.world      *     position;
    out_position      = uniforms.view       * out_position;
    out_position      = uniforms.projection * out_position;

    gl_Position = out_position;

    int_color = color;
}
