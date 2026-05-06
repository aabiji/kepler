#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent; // xyz=tangent, w=handedness

uniform mat4 view;
uniform mat4 projection;
uniform bool buffer_mode;

out vec2 texture_coords;
out vec4 obj_color;
out vec3 obj_pos;
out mat3 tbn_matrix;
flat out uint instance_index;

struct InstanceData {
    mat4 model_matrix;
    mat4 normal_matrix;
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
        obj_pos = world_pos;
    } else {
        vec4 p = d.model_matrix * vec4(in_pos, 1.0);
        gl_Position = projection * view * p;
        obj_pos = vec3(p);
    }

    // No need for lighting calculations when writing a framebuffer
    if (!buffer_mode) {
        // First map the vectors from object space to world space, then compute a
        // matrix to map from tangent space (surface of the object) to world space
        mat3 nm = mat3(d.normal_matrix);
        vec3 N = normalize(nm * in_normal);
        vec3 T = normalize(nm * in_tangent.xyz);
        // Make sure the vectors are perpendicular to the normal
        T = normalize(T - N * dot(T, N));
        vec3 B = cross(N, T) * in_tangent.w;
        tbn_matrix = mat3(T, B, N);
    }

    obj_color = d.color;
    texture_coords = in_uv;
    instance_index = gl_InstanceID;
}
