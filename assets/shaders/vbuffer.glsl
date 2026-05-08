#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent;

uniform mat4 view;
uniform mat4 projection;

struct InstanceData {
    mat4 model_matrix;
    mat4 normal_matrix;
    vec4 color;
    int is_2d;
};

layout(binding = 0, std430) readonly buffer b {
    InstanceData data[];
};

flat out uint instance_index;

void main() {
    InstanceData d = data[gl_InstanceID];

    // Adding 1 to differentiate between the background and the indexes
    instance_index = gl_InstanceID + 1;

    // Render 2D shapes the same way regardless of camera orientation.
    // Objects that are further away will appear smaller.
    float size = 0.005;
    vec3 world_pos = d.model_matrix[3].xyz;
    vec4 view_pos = view * vec4(world_pos, 1.0);
    view_pos.xy += in_pos.xy * size;
    gl_Position = projection * view_pos;
}
