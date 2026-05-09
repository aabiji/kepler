#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <future>
#include <glm/glm.hpp>
#include <set>
#include <thread>

#include "camera.h"
#include "mesh.h"
#include "panel.h"
#include "satellite.h"
#include "shader.h"
#include "texture.h"

class GLFWContext {
public:
  GLFWContext();
  ~GLFWContext();
};

struct InputState {
  float yscroll;
  glm::ivec2 window_size;
  glm::vec2 cursor_delta;
  glm::vec2 prev_cursor;
  std::set<int> keys;
  bool mouse_pressed;
  bool resized;

  InputState()
      : yscroll(0), cursor_delta(0.0), prev_cursor(0.0), mouse_pressed(false),
        resized(false) {}
};

class Visualizer {
public:
  ~Visualizer();
  Visualizer(int width, int height);
  void run();

private:
  void create_window(int width, int height);
  void set_callbacks();
  void init_components();
  void render_ui();
  void render_scene();
  void render_satellites();

  GLFWwindow *window;
  InputState state;

  glm::mat4 projection;
  glm::vec3 sun_pos;
  std::string search_error;
  SharedInstanceData circle_instances;
  std::vector<InstanceData> globe_instances;

  double constellation_time_step;
  unsigned int selected_satellite;
  std::vector<Satellite> satellites;

  std::jthread simulation_thread;
  std::future<std::vector<Satellite>> load_future;

  InfoPanel ui;
  Camera camera;
  Shader main_shader;
  Shader cubemap_shader;
  Shader framebuffer_shader;
  Skybox skybox;
  Texture cubemap_texture;
  Texture earth_texture;
  Texture earth_normal_map;
  Texture earth_specular_map;
  InstancedMesh globe;
  InstancedMesh circles;
  Framebuffer framebuffer;
};
