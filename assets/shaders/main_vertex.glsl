#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

uniform mat4 view;
uniform mat4 projection;

out vec2 texture_coords;
out vec3 normal;
out vec4 obj_color;
out vec3 obj_position;

struct InstanceData {
    mat4 model_matrix;
    vec4 color;
    int is_2d;
};

layout(binding = 0, std430) readonly buffer b {
    InstanceData data[];
};

void main() {
    InstanceData d = data[gl_InstanceID];

    // Render 2D shapes the same way regardless of camera orientation.
    // Objects that are further away will appear smaller.
    if (d.is_2d == 1) {
        float size = 0.01;
        vec3 world_pos = d.model_matrix[3].xyz;
        vec4 view_pos = view * vec4(world_pos, 1.0);

        view_pos.xy += in_pos.xy * size;
        gl_Position = projection * view_pos;
        obj_position = world_pos;
    } else {
        vec4 p = d.model_matrix * vec4(in_pos, 1.0);
        gl_Position = projection * view * p;
        obj_position = vec3(p);
    }

    obj_color = d.color;
    texture_coords = in_uv;
    normal = in_normal;
}
