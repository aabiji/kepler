#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_interpolation.hpp>
#include <mutex>
#include <vector>

struct alignas(16) Vertex {
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 normal;
  glm::vec4 tangent;
};

// NOTE: See vertex.glsl
struct alignas(16) InstanceData {
  glm::mat4 model_matrix;
  glm::mat4 normal_matrix;
  glm::vec4 color;
  int is_2d;
  InstanceData(glm::vec3 position, glm::vec3 scale);
  InstanceData() {}
};

struct SharedInstances {
  std::vector<InstanceData> data;
  std::mutex mutex;
};

struct Skybox {
  ~Skybox();
  void init();
  void render();
  unsigned int vao, vbo;
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
