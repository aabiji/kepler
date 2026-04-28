#include <cmath>
#include <glad/glad.h>

#include "mesh.h"

void InstancedMesh::init_buffers(int num_instances) {
  glGenVertexArrays(1, &vao);
  glCreateBuffers(1, &vbo);
  glCreateBuffers(1, &ebo);
  glCreateBuffers(1, &ssbo);

  glBindVertexArray(vao);
  glNamedBufferStorage(ssbo, sizeof(InstanceData) * num_instances, nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, position));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, uv));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, normal));
  glEnableVertexAttribArray(2);
}

InstancedMesh::~InstancedMesh() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &ssbo);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

void InstancedMesh::render() {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                  data.size() * sizeof(InstanceData), data.data());

  glBindVertexArray(vao);
  glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0,
                          data.size());
}

InstancedMesh generate_unit_sphere(int longitudes, int lattitudes) {
  InstancedMesh mesh;

  // Add vertices
  for (int i = 0; i <= lattitudes; i++) {
    float lattitude_angle =
        (M_PI / 2.0) - i * (M_PI / (float)lattitudes); // -pi/2 to pi/2
    float xy = cos(lattitude_angle);
    float z = sin(lattitude_angle);

    for (int j = 0; j <= longitudes; j++) {
      float longitude_angle = j * (2.0 * M_PI / (float)longitudes);

      Vertex v;
      v.position =
          glm::vec3(xy * cos(longitude_angle), z, xy * sin(longitude_angle));
      v.uv = glm::vec2((float)j / (float)longitudes,
                       1.0 - (float)i / (float)lattitudes);
      v.normal = v.position;

      mesh.vertices.push_back(v);
    }
  }

  // Add indices
  for (int i = 0; i < lattitudes; i++) {
    int top = i * (longitudes + 1);
    int bottom = top + longitudes + 1;

    for (int j = 0; j < longitudes; j++, top++, bottom++) {
      if (i != 0) {
        mesh.indices.push_back(top);
        mesh.indices.push_back(bottom);
        mesh.indices.push_back(top + 1);
      }

      if (i != (lattitudes - 1)) {
        mesh.indices.push_back(top + 1);
        mesh.indices.push_back(bottom);
        mesh.indices.push_back(bottom + 1);
      }
    }
  }

  return mesh;
}

InstancedMesh generate_circle_mesh(int num_fans) {
  InstancedMesh mesh;

  Vertex v;
  v.uv = v.normal = glm::vec3(0.0, 0.0, 0.0);
  v.position = glm::vec3(0.0, 0.0, 0.0);
  mesh.vertices.push_back(v);

  for (int i = 0; i < num_fans; i++) {
    Vertex v;
    float angle = i * ((2.0 * M_PI) / (float)num_fans);
    v.position = glm::vec3(std::cos(angle), std::sin(angle), 0);
    v.uv = v.normal = glm::vec3(0.0, 0.0, 0.0);
    mesh.vertices.push_back(v);
  }

  for (int i = 1; i <= num_fans; i++) {
    mesh.indices.push_back(0);
    mesh.indices.push_back(i);
    mesh.indices.push_back((i % num_fans) + 1);
  }

  return mesh;
}