#version 460 core

layout(location = 0) in vec3 in_pos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(in_pos, 1.0);
}
