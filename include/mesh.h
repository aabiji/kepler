#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_interpolation.hpp>

struct Vertex {
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 normal;
};

// NOTE: See vertex.glsl
struct InstanceData {
  glm::mat4 model_matrix;
  glm::vec4 color;
};

struct InstancedMesh {
  ~InstancedMesh();

  void render();
  void init_buffers(int num_instances);

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<InstanceData> data;
  unsigned int vao, vbo, ebo, ssbo;
};

InstancedMesh generate_unit_sphere(int longitudes, int lattitudes);
InstancedMesh generate_circle_mesh(int num_fans);