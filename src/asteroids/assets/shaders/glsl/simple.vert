#version 450

layout(location=0) in vec4 position;
layout(location=1) in vec4 color;

layout(binding = 0) uniform Uniforms {
    mat4 projection;
    mat4 view;
    mat4 world;
} uniforms;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location=0) out vec4 int_color;

void main() {
    vec4 out_position =     position * uniforms.world;
    out_position      = out_position * uniforms.view;
    out_position      = out_position * uniforms.projection;

    gl_Position = out_position;
    int_color = color;
}
