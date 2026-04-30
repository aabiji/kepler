#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_interpolation.hpp>

struct Skybox {
  ~Skybox();
  Skybox();
  void render();
  unsigned int vao, vbo;
};

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
  void init_buffers();

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<InstanceData> data;
  unsigned int vao, vbo, ebo, ssbo;
};

InstancedMesh create_unit_sphere(int longitudes, int lattitudes);
InstancedMesh create_circle_mesh(int num_fans);
