#version 450

layout(location=0) in vec3 in_norm;

layout(location=0) out vec4 out_Color;

layout(set = 0, binding = 1) uniform Uniforms {
    vec4 color;
} uniforms;


void main()
{
    vec3 light_pos    = vec3(0.5, 0.25, -1);
    out_Color = vec4(clamp(dot(in_norm, normalize(light_pos)), 0.0f, 1.0f));
}
