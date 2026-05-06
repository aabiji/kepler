#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "debug.h"
#include "mesh.h"
#include "satellite.h"
#include "viz.h"

// This is done so that GLFW can be terminated after
// the Visualizer deconstructor is called
GLFWContext::GLFWContext() { glfwInit(); }
GLFWContext::~GLFWContext() { glfwTerminate(); }

Visualizer::Visualizer(int width, int height) {
  create_window(width, height);
  set_callbacks();
  init_scene_objects();
}

Visualizer::~Visualizer() {
  simulation_thread.request_stop();
  simulation_thread.join();
}

void Visualizer::create_window(int width, int height) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window =
      glfwCreateWindow(width, height, "LEO Visualization", nullptr, nullptr);
  if (window == nullptr)
    THROW_ERROR("Failed to create window");

  glfwMakeContextCurrent(window);
  if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
    THROW_ERROR("Failed to load the OpenGL context");

  state.projection =
      glm::perspective((float)std::numbers::pi / 4.0f,
                       (float)width / (float)height, 0.1f, 100.0f);
  glEnable(GL_DEPTH_TEST);
}

void Visualizer::set_callbacks() {
  glfwSetWindowUserPointer(window, &state);

  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(gl_debug_callback, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                        GL_TRUE);

  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *window, int button, int action, int mods) {
        InputState *state =
            static_cast<InputState *>(glfwGetWindowUserPointer(window));
        if (button == GLFW_MOUSE_BUTTON_LEFT)
          state->mouse_pressed = action == GLFW_PRESS;
        (void)mods;
      });

  glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode,
                                int action, int mods) {
    (void)scancode;
    (void)mods;
    InputState *state =
        static_cast<InputState *>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
      state->keys.insert(key);
    else
      state->keys.erase(key);
  });

  glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int w, int h) {
    InputState *state =
        static_cast<InputState *>(glfwGetWindowUserPointer(window));
    state->projection = glm::perspective((float)std::numbers::pi / 4.0f,
                                         (float)w / (float)h, 0.1f, 100.0f);
    glViewport(0, 0, w, h);
  });

  glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
    InputState *state =
        static_cast<InputState *>(glfwGetWindowUserPointer(window));
    state->cursor_delta =
        glm::vec2(x - state->prev_cursor.x, y - state->prev_cursor.y);
    state->prev_cursor = glm::vec2(x, y);
  });

  glfwSetScrollCallback(
      window, [](GLFWwindow *window, double xoffset, double yoffset) {
        InputState *state =
            static_cast<InputState *>(glfwGetWindowUserPointer(window));
        state->yscroll = yoffset;
        (void)xoffset;
      });
}

void Visualizer::init_scene_objects() {
  main_shader.init("../assets/shaders/main_vertex.glsl",
                   "../assets/shaders/main_fragment.glsl");
  cubemap_shader.init("../assets/shaders/cubemap_vertex.glsl",
                      "../assets/shaders/cubemap_fragment.glsl");
  cubemap_texture.init(
      {"../assets/textures/cubemap/px.png", "../assets/textures/cubemap/nx.png",
       "../assets/textures/cubemap/py.png", "../assets/textures/cubemap/ny.png",
       "../assets/textures/cubemap/pz.png",
       "../assets/textures/cubemap/nz.png"});
  earth_texture.init({"../assets/textures/earth/day.jpg"});
  earth_normal_map.init({"../assets/textures/earth/normal.png"});
  earth_specular_map.init({"../assets/textures/earth/specular.png"});

  circles = create_circle_mesh(10);
  globe = create_unit_sphere(32, 32);
  skybox.init();

  constellation_time_step = 1; // Propagate every 1 second
  sun_pos = glm::vec3((1.0 / 6371.0) * 149600000.0, 0.0, 0.0);
  globe_instances.push_back(InstanceData(glm::vec3(0.0), glm::vec3(1.0)));

  simulation_thread =
      std::jthread(simulate_satellites, "../assets/starlink.csv",
                   std::ref(circle_instances));
}

void Visualizer::run() {
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  state.prev_cursor = glm::vec2(x, y);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // clang-format off
    if (state.keys.contains(GLFW_KEY_W)) camera.move_vertically(true);
    if (state.keys.contains(GLFW_KEY_S)) camera.move_vertically(false);
    if (state.keys.contains(GLFW_KEY_A)) camera.rotate_position(false);
    if (state.keys.contains(GLFW_KEY_D)) camera.rotate_position(true);
    if (state.mouse_pressed) camera.rotate_orientation(state.cursor_delta, 0.001);
    if (state.yscroll != 0) camera.zoom(state.yscroll < 0);
    // clang-format on

    state.yscroll = 0;
    state.cursor_delta = glm::vec2(0.0);
    render_scene();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

void Visualizer::render_scene() {
  glm::mat4 v = camera.view_matrix();
  glm::mat4 v_no_translation = glm::mat4(glm::mat3(v));

  main_shader.use();
  main_shader.set<glm::mat4>("view", v);
  main_shader.set<glm::mat4>("projection", state.projection);
  main_shader.set<glm::vec3>("view_pos", camera.get_position());
  main_shader.set<glm::vec3>("sun_pos", sun_pos);
  main_shader.set<int>("planet_texture", 0);
  main_shader.set<int>("planet_normal_map", 1);
  main_shader.set<int>("planet_specular_map", 2);
  main_shader.set<bool>("use_texture", true);

  earth_texture.use(0);
  earth_normal_map.use(1);
  earth_specular_map.use(2);
  globe.render(globe_instances);

  main_shader.set<bool>("use_texture", false);
  {
    std::lock_guard<std::mutex> guard(circle_instances.mutex);
    circles.render(circle_instances.data);
  }

  // Render the skybox
  cubemap_shader.use();
  cubemap_shader.set<glm::mat4>("projection", state.projection);
  cubemap_shader.set<glm::mat4>("view", v_no_translation);
  glDepthFunc(GL_LEQUAL);
  cubemap_texture.use(0);
  skybox.render();
  glDepthFunc(GL_LESS);
}
