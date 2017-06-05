#version 450

layout(location=0) in vec4 position;
layout(location=1) in vec3 norm;

layout(binding = 0) uniform PerFrameUniforms {
    mat4 projection;
    mat4 view;
} frame_uniforms;

layout(binding = 1) uniform PerModelUniforms {
    mat4 world;
} model_uniforms;

layout(location=0) out vec3 out_norm;

void main() {
    vec4 out_position = model_uniforms.world      *     position;
    out_position      = frame_uniforms.view       * out_position;
    out_position      = frame_uniforms.projection * out_position;

    gl_Position = out_position;

    out_norm = (model_uniforms.world * vec4(norm,0)).xyz;
}
