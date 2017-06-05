#version 450

layout(location=0) in vec3 in_norm;

layout(location=0) out vec4 out_Color;

layout(set = 0, binding = 1) uniform Uniforms {
    vec4 color;
} uniforms;


void main()
{
    float ambient_factor = 0.1f;
    vec3 light_pos = vec3(0.5, 0.25, -1);
    float n_dot_l = clamp(dot(in_norm, normalize(light_pos)), 0.0f, 1.0f);

    vec3 brown = vec3(0.4f, 0.2f, 0.0f);
    vec3 ambient_color = brown * ambient_factor;
    vec3 diffuse_color = brown * (1-ambient_factor) * n_dot_l;

    out_Color = vec4(ambient_color + diffuse_color, 1.0f);
}
