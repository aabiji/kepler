#version 460 core

in vec2 texture_coords;
in vec4 obj_color;
in vec3 obj_pos;
in mat3 tbn_matrix;
flat in uint instance_index;

uniform vec3 sun_pos;
uniform vec3 view_pos; // In world space
uniform bool use_texture;
uniform uint selected_index;

uniform sampler2D planet_texture;
uniform sampler2D planet_normal_map;
uniform sampler2D planet_specular_map;

layout(location = 0) out vec4 fragment_color;
layout(location = 1) out uint output_value;

vec4 phong_lighting() {
    bool selected = selected_index == instance_index;
    // TODO: draw an outline if selected

    vec4 pixel = use_texture ? texture(planet_texture, texture_coords) : obj_color;

    vec3 normal_value = texture(planet_normal_map, texture_coords).rgb;
    vec3 normal;
    if (length(tbn_matrix[0]) < 0.1) {
        // Default to the geometric normal at the sphere poles
        normal = normalize(obj_pos);
    } else {
        // Range from -1 to 1
        normal = normalize(tbn_matrix * (normal_value * 2.0 - 1.0));
    }

    vec4 light_color = vec4(1.0, 1.0, 1.0, 1.0);
    vec3 light_dir = normalize(sun_pos - obj_pos);
    vec3 view_dir = normalize(view_pos - obj_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);

    float shininess = 16.0, ambient_strength = 0.3;
    float intensity = texture(planet_specular_map, texture_coords).r;
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    float diffuse_strength = max(dot(normal, light_dir), 0.0);

    vec4 diffuse = diffuse_strength * light_color;
    vec4 ambient = ambient_strength * light_color;
    vec4 specular = intensity * spec * light_color;
    return (ambient + diffuse + specular) * pixel;
}

void main() {
    fragment_color = phong_lighting();
    output_value = instance_index;
}
