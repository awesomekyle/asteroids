#version 450

layout(location=0) in vec4 position;
layout(location=1) in vec4 color;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location=0) out vec4 int_color;

void main() {
    gl_Position = position;
    int_color = color;
}
