#version 460 core

in vec2 texture_coords;
in vec3 normal;
in vec4 color;

uniform bool use_texture;
uniform sampler2D planet_texture;

out vec4 fragment_color;

void main() {
  fragment_color = use_texture ?  texture(planet_texture, texture_coords) : color;
}