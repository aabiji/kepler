#include <assert.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// TODO:
// - Fix icosphere UV mapping
// - Tie up some loose ends (resizing, error handling, scale normal by normal
// matrix, basic phong lighting, etc) and refactor

unsigned int load_shader(const char *path, int type) {
  auto size = std::filesystem::file_size(path);
  std::string contents(size, '\0');
  std::ifstream file(path);
  assert(file.good() && file.is_open());
  file.read(&contents[0], size);

  const char *data = contents.c_str();
  unsigned int id = glCreateShader(type);
  glShaderSource(id, 1, &data, nullptr);
  glCompileShader(id);

  int success = 0;
  char info_log[1024];
  glGetShaderiv(id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(id, 1024, nullptr, info_log);
    std::cout << path << " -> " << info_log << "\n";
    std::exit(-1);
  }

  return id;
}

class Shader {
public:
  explicit Shader(const char *vshader_path, const char *fshader_path) {
    unsigned int vertex = load_shader(vshader_path, GL_VERTEX_SHADER);
    unsigned int fragment = load_shader(fshader_path, GL_FRAGMENT_SHADER);

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success = 0;
    char info_log[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(program, 1024, nullptr, info_log);
      std::cout << "Link error -> " << info_log << "\n";
      std::exit(-1);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  ~Shader() { glDeleteProgram(program); }

  void use() { glUseProgram(program); }

  void set_vec3(const char *name, glm::vec3 &value) {
    glUniform3fv(glGetUniformLocation(program, name), 1, glm::value_ptr(value));
  }

  void set_mat4(const char *name, glm::mat4 &value) {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE,
                       glm::value_ptr(value));
  }

private:
  unsigned int program;
};

struct vec3hash {
  std::size_t operator()(const glm::vec3 &v) const {
    return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1) ^
           (std::hash<float>()(v.z) << 2);
  }
};

struct Vertex {
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 normal;

  Vertex(glm::vec3 p) : position(p), uv(0.0), normal(0.0) {}
};

class Mesh {
public:
  ~Mesh();
  explicit Mesh(std::vector<Vertex> v, std::vector<unsigned int> i);

  void render();
  void rotate(float dx, float dy);
  glm::mat4 model_matrix();

private:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  unsigned int vao, vbo, ebo;
  glm::quat current_rotation;
};

Mesh::Mesh(std::vector<Vertex> v, std::vector<unsigned int> i)
    : vertices(v), indices(i), current_rotation(1.0, 0.0, 0.0, 0.0) {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);
  glBindVertexArray(vao);

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

Mesh::~Mesh() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

void Mesh::render() {
  glBindVertexArray(vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

glm::mat4 Mesh::model_matrix() {
  // translation * rotation * scale
  return glm::mat4(current_rotation);
}

void Mesh::rotate(float dx, float dy) {
  float sensitivity = 0.1;

  glm::vec3 world_up = glm::vec3(0.0, 1.0, 0.0);
  glm::vec3 world_right = current_rotation * glm::vec3(1.0, 0.0, 0.0);

  glm::quat yaw = glm::angleAxis(dx * sensitivity, world_up);
  glm::quat pitch = glm::angleAxis(dy * sensitivity, world_right);

  current_rotation = glm::normalize(yaw * pitch * current_rotation);
}

Mesh generate_unit_sphere(int depth) {
  std::vector<Vertex> vertices;
  std::unordered_map<glm::vec3, unsigned int, vec3hash> vertex_map;

  auto add_vertex = [&](glm::vec3 v) {
    v = glm::normalize(v);
    int index = vertices.size();
    vertices.push_back(Vertex(v));
    vertex_map.insert({v, index});
    return index;
  };

  auto midpoint = [&](int p1, int p2) {
    glm::vec3 v = (vertices[p1].position + vertices[p2].position) / 2.0f;
    return vertex_map.count(v) ? vertex_map[v] : add_vertex(v);
  };

  // Initial icosahedron points
  double p = (1.0 + std::sqrt(5.0)) / 2.0;
  std::vector<glm::vec3> initial_vertices = {
      glm::vec3(-1, p, 0),  glm::vec3(1, p, 0),   glm::vec3(-1, -p, 0),
      glm::vec3(1, -p, 0),  glm::vec3(0, -1, p),  glm::vec3(0, 1, p),
      glm::vec3(0, -1, -p), glm::vec3(0, 1, -p),  glm::vec3(p, 0, -1),
      glm::vec3(p, 0, 1),   glm::vec3(-p, 0, -1), glm::vec3(-p, 0, 1)};
  for (glm::vec3 v : initial_vertices)
    add_vertex(v);

  // Each triangle is grouped by 3 indices
  std::vector<unsigned int> indices = {
      0, 11, 5,  0, 5,  1, 0, 1, 7, 0, 7,  10, 0, 10, 11, 1, 5, 9, 5, 11,
      4, 11, 10, 2, 10, 7, 6, 7, 1, 8, 3,  9,  4, 3,  4,  2, 3, 2, 6, 3,
      6, 8,  3,  8, 9,  4, 9, 5, 2, 4, 11, 6,  2, 10, 8,  6, 7, 9, 8, 1};

  // Subdivide the vertices
  for (int j = 0; j < depth; j++) {
    std::vector<unsigned int> new_indices;

    for (size_t i = 0; i < indices.size(); i += 3) {
      int a = midpoint(indices[i], indices[i + 1]);
      int b = midpoint(indices[i + 1], indices[i + 2]);
      int c = midpoint(indices[i + 2], indices[i]);

      new_indices.push_back(indices[i]);
      new_indices.push_back(a);
      new_indices.push_back(c);

      new_indices.push_back(indices[i + 1]);
      new_indices.push_back(b);
      new_indices.push_back(a);

      new_indices.push_back(indices[i + 2]);
      new_indices.push_back(c);
      new_indices.push_back(b);

      new_indices.push_back(a);
      new_indices.push_back(b);
      new_indices.push_back(c);
    }

    indices = new_indices;
  }

  // Spherically project UV coordinates
  for (size_t i = 0; i < vertices.size(); i++) {
    glm::vec3 p = vertices[i].position;
    vertices[i].uv = glm::vec2(0.5 + std::atan2(p.z, p.x) / (2.0 * M_PI),
                               0.5 - std::asin(p.y) / M_PI);
    vertices[i].normal = p; // The normal is the position on a unit sphere
  }

  return Mesh(vertices, indices);
}

class Camera {
public:
  explicit Camera(float aspect_ratio) {
    projection =
        glm::perspective((float)M_PI / 4.0f, aspect_ratio, 0.1f, 100.0f);

    up = glm::vec3(0.0, 1.0, 0.0);
    pos = glm::vec3(0.0, 0.0, 3.0);
    front = glm::vec3(0.0, 0.0, -1.0); // -Z goes into the screen
  }

  void move(float direction) { pos += front * direction; }

  glm::mat4 projection_matrix() { return projection; }
  glm::mat4 view_matrix() { return glm::lookAt(pos, pos + front, up); }

private:
  glm::vec3 up, pos, front;
  glm::mat4 projection;
};

int main() {
  // auto satellites = read_satellite_data("../data/starlink.csv");

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int window_width = 900, window_height = 700;
  GLFWwindow *window = glfwCreateWindow(window_width, window_height,
                                        "LEO Visualization", nullptr, nullptr);
  assert(window != nullptr);

  glfwMakeContextCurrent(window);
  assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) != 0);

  glViewport(0, 0, window_width, window_height);
  glEnable(GL_DEPTH_TEST);

  float aspect_ratio = (float)window_width / (float)window_height;
  Camera camera(aspect_ratio);

  {
    Shader shader("../src/vertex.glsl", "../src/fragment.glsl");
    Mesh sphere = generate_unit_sphere(3);

    const char *path = "../data/test.png";
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 3);
    assert(pixels != nullptr && channels == 3);

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);

    double prev_x = 0, prev_y = 0;
    glfwGetCursorPos(window, &prev_x, &prev_y);

    while (!glfwWindowShouldClose(window)) {
      if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        break;

      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.move(-1);

      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.move(1);

      double x, y;
      glfwGetCursorPos(window, &x, &y);
      bool mouse_down =
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      if (mouse_down)
        sphere.rotate(x - prev_x, prev_y - y);
      prev_x = x;
      prev_y = y;

      glClearColor(0.0, 0.0, 0.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 p = camera.projection_matrix();
      glm::mat4 v = camera.view_matrix();
      glm::mat4 model = sphere.model_matrix();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);

      shader.use();
      shader.set_mat4("projection", p);
      shader.set_mat4("view", v);
      shader.set_mat4("model", model);
      sphere.render();

      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    glDeleteTextures(1, &texture);
  }

  glfwTerminate();
  return 0;
}
