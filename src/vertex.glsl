#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 texture_coords;
out vec3 normal;

void main() {
    gl_Position = projection * view * model * vec4(in_pos, 1.0);
    texture_coords = in_uv;
    normal = in_normal;
}