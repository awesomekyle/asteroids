#version 450

layout(location=0) in vec4 position;
layout(location=1) in vec3 norm;

layout(set = 0,binding = 0) uniform Uniforms {
    mat4 projection;
    mat4 view;
    mat4 world;
} uniforms;

layout(location=0) out vec3 out_norm;

void main() {
    vec4 out_position = uniforms.world      *     position;
    out_position      = uniforms.view       * out_position;
    out_position      = uniforms.projection * out_position;

    gl_Position = out_position;

    out_norm = (uniforms.world * vec4(norm,0)).xyz;
}
