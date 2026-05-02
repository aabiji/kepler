#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_interpolation.hpp>

struct Skybox {
  ~Skybox();
  void init();
  void render();
  unsigned int vao, vbo;
};

struct alignas(16) Vertex {
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 normal;
};

// NOTE: See vertex.glsl
struct alignas(16) InstanceData {
  glm::mat4 model_matrix;
  glm::vec4 color;
  int is_2d;
  InstanceData(glm::mat4 m, glm::vec4 c)
      : model_matrix(m), color(c), is_2d(false) {}
  InstanceData() {}
};

class InstancedMesh {
public:
  ~InstancedMesh();
  InstancedMesh();
  explicit InstancedMesh(std::vector<Vertex> vertices,
                         std::vector<unsigned int> indices);

  // Only allow moving, not copying
  InstancedMesh(const InstancedMesh &) = delete;
  InstancedMesh &operator=(const InstancedMesh &) = delete;
  InstancedMesh(InstancedMesh &&other) noexcept;
  InstancedMesh &operator=(InstancedMesh &&other) noexcept;

  void render(std::vector<InstanceData> &data);

private:
  bool initialized;
  int num_indices, ssbo_size;
  unsigned int vao, vbo, ebo, ssbo;
};

InstancedMesh create_unit_sphere(int longitudes, int lattitudes);
InstancedMesh create_circle_mesh(int num_fans);
