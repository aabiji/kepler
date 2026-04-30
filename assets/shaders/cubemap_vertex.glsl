#version 460 core

layout(location = 0) in vec3 in_pos;
out vec3 texture_coords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    texture_coords = in_pos;
    vec4 pos = projection * view * vec4(in_pos, 1.0);
    // This forces the z component to be 1.0, making the skybox render behind everything else
    gl_Position = pos.xyww;
}
