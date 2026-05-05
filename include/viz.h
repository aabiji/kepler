#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <set>
#include <thread>

#include "camera.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

class GLFWContext {
public:
  GLFWContext();
  ~GLFWContext();
};

struct InputState {
  float yscroll;
  glm::vec2 cursor_delta;
  glm::vec2 prev_cursor;
  std::set<int> keys;
  glm::mat4 projection;
  bool mouse_pressed;

  InputState()
      : yscroll(0), cursor_delta(0.0), prev_cursor(0.0), projection(1.0),
        mouse_pressed(false) {}
};

class Visualizer {
public:
  ~Visualizer();
  Visualizer(int width, int height);
  void run();

private:
  void create_window(int width, int height);
  void set_callbacks();
  void init_scene_objects();
  void render_scene();

  GLFWwindow *window;
  InputState state;

  glm::vec3 sun_pos;
  double earth_scale;
  double constellation_time_step;
  std::vector<InstanceData> globe_instances;
  std::vector<InstanceData> circle_instances;
  std::jthread simulation_thread;

  Camera camera;
  Shader main_shader;
  Shader cubemap_shader;
  Skybox skybox;
  Texture cubemap_texture;
  Texture earth_texture;
  Texture earth_normal_map;
  Texture earth_specular_map;
  InstancedMesh globe;
  InstancedMesh circles;
};
