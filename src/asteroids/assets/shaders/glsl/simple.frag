#version 450

layout(location = 0) in vec4 int_color;

layout(location = 0) out vec4 out_Color;

void main() {
  out_Color = int_color;
}
