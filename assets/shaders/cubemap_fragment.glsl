#version 460 core

in vec3 texture_coords;
out vec4 fragment_color;
uniform samplerCube cubemap;

void main() {
    fragment_color = texture(cubemap, texture_coords);
}
