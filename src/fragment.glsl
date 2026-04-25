#version 460 core

in vec2 texture_coords;
in vec3 normal;
uniform sampler2D planet_texture;

out vec4 fragment_color;

void main() {
    fragment_color = texture(planet_texture, texture_coords);
}