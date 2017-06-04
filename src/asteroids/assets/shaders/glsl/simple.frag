#version 450

layout(location = 0) in vec4 int_color;

layout(location = 0) out vec4 out_Color;

layout(set = 0, binding = 1) uniform Uniforms {
    vec4 color;
} uniforms;


void main()
{
    out_Color = int_color * uniforms.color;
}
