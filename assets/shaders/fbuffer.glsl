#version 460 core

flat in uint instance_index;
out uint output_value;

void main() {
    output_value = instance_index;
}
