#version 450

layout(location=0) in vec4 position;
layout(location=1) in vec3 norm;

layout(binding = 0) uniform PerFrameUniforms {
    mat4 projection;
    mat4 view;
    mat4 viewproj;
} frame_uniforms;

layout(binding = 1) uniform PerModelUniforms {
    mat4 wvp;
    mat4 world;
} model_uniforms;

layout(location=0) out vec3 out_norm;

void main()
{
    gl_Position = model_uniforms.wvp * position;

    out_norm = (model_uniforms.world * vec4(norm,0)).xyz;
}
