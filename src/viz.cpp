#include <future>
#include <glad/glad.h>

#include "debug.h"
#include "viz.h"

// This is done so that GLFW can be terminated after
// the Visualizer deconstructor is called
GLFWContext::GLFWContext() { glfwInit(); }
GLFWContext::~GLFWContext() { glfwTerminate(); }

Visualizer::Visualizer(int width, int height) {
  create_window(width, height);
  set_callbacks();
  init_components();
}

Visualizer::~Visualizer() {
  if (simulation_thread.joinable()) {
    simulation_thread.request_stop();
    simulation_thread.join();
  }
}

void Visualizer::create_window(int width, int height) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

  window =
      glfwCreateWindow(width, height, "LEO Visualization", nullptr, nullptr);
  if (window == nullptr)
    THROW_ERROR("Failed to create window");

  glfwMakeContextCurrent(window);
  if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
    THROW_ERROR("Failed to load the OpenGL context");

  state.window_size = glm::ivec2(width, height);
  projection = glm::perspective((float)std::numbers::pi / 4.0f,
                                (float)width / (float)height, 0.1f, 100.0f);
}

void Visualizer::set_callbacks() {
  glfwSetWindowUserPointer(window, &state);

  glEnable(GL_DEPTH_TEST);
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
    state->resized = true;
    state->window_size = glm::ivec2(w, h);
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

void Visualizer::init_components() {
  ui.init(window);

  auto spath = [](const char *name) {
    return std::format("../assets/shaders/{}.glsl", name);
  };
  main_shader.init(spath("vmain"), spath("fmain"));
  cubemap_shader.init(spath("vcubemap"), spath("fcubemap"));
  framebuffer_shader.init(spath("vbuffer"), spath("fbuffer"));

  std::string folder = "cubemap";
  auto tpath = [&](const char *filename) {
    return std::format("../assets/textures/{}/{}", folder, filename);
  };
  cubemap_texture.init({tpath("px.png"), tpath("nx.png"), tpath("py.png"),
                        tpath("ny.png"), tpath("pz.png"), tpath("nz.png")});

  folder = "earth";
  earth_texture.init({tpath("day.jpg")});
  earth_normal_map.init({tpath("normal.png")});
  earth_specular_map.init({tpath("specular.png")});

  circles = create_circle_mesh(10);
  globe = create_unit_sphere(32, 32);
  skybox.init();

  constellation_time_step = 1; // Propagate every 1 second
  sun_pos = glm::vec3((1.0 / 6371.0) * 149600000.0, 0.0, 0.0);
  globe_instances.push_back(
      InstanceData(glm::vec3(0.0), glm::vec3(1.0), false));
  selected_satellite = 0;

  framebuffer.resize(state.window_size.x, state.window_size.y);

  load_future = std::async(std::launch::async, []() {
    return load_satellite_data("../assets/satellites.csv");
  });
}

void Visualizer::run() {
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  state.prev_cursor = glm::vec2(x, y);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, state.window_size.x, state.window_size.y);

    // clang-format off
    if (state.keys.contains(GLFW_KEY_W)) camera.move_vertically(true);
    if (state.keys.contains(GLFW_KEY_S)) camera.move_vertically(false);
    if (state.keys.contains(GLFW_KEY_A)) camera.rotate_position(false);
    if (state.keys.contains(GLFW_KEY_D)) camera.rotate_position(true);

    if (state.resized) {
      int w = state.window_size.x, h = state.window_size.y;
      projection = glm::perspective((float)std::numbers::pi / 4.0f,
                                           (float)w / (float)h, 0.1f, 100.0f);
      framebuffer.resize(w, h);
    }

    if (!ui.active()) {
      if (state.yscroll != 0) camera.zoom(state.yscroll < 0);

      if (state.mouse_pressed) {
        camera.rotate_orientation(state.cursor_delta, 0.001);
        // Opengl defines (0, 0) to be the bottom left
        int y = state.window_size.y - state.prev_cursor.y;
        selected_satellite = framebuffer.read_value(state.prev_cursor.x, y);
      }
    }

    if (load_future.valid()) {
      auto status = load_future.wait_for(std::chrono::seconds(0));
      if (status == std::future_status::ready) {
        satellites = load_future.get();
        simulation_thread = std::jthread(simulate_satellites, satellites, std::ref(circle_instances));
      }
    }
    // clang-format on

    state.yscroll = 0;
    state.cursor_delta = glm::vec2(0.0);
    state.resized = false;
    render_scene();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

void Visualizer::render_satellites() {
  std::lock_guard<std::mutex> guard(circle_instances.mutex);

  // Render circles indexes to the framebuffer
  framebuffer.bind(true);
  framebuffer_shader.use();
  framebuffer_shader.set<glm::mat4>("view", camera.view_matrix());
  framebuffer_shader.set<glm::mat4>("projection", projection);

  glViewport(0, 0, state.window_size.x, state.window_size.y);
  framebuffer.clear();
  circles.render(circle_instances.data);
  framebuffer.bind(false);

  // Render circles to the screen
  main_shader.use();
  circles.render(circle_instances.data);
}

void Visualizer::render_scene() {
  main_shader.use();
  main_shader.set<unsigned int>("selected_index", selected_satellite);
  main_shader.set<glm::mat4>("view", camera.view_matrix());
  main_shader.set<glm::mat4>("projection", projection);
  main_shader.set<glm::vec3>("view_pos", camera.get_position());
  main_shader.set<glm::vec3>("sun_pos", sun_pos);
  main_shader.set<int>("planet_texture", 0);
  main_shader.set<int>("planet_normal_map", 1);
  main_shader.set<int>("planet_specular_map", 2);

  // Render the globe
  main_shader.set<bool>("use_texture", true);
  earth_texture.use(0);
  earth_normal_map.use(1);
  earth_specular_map.use(2);
  globe.render(globe_instances);
  main_shader.set<bool>("use_texture", false);

  render_satellites();

  // Render the skybox
  glm::mat4 view_no_translation = glm::mat4(glm::mat3(camera.view_matrix()));
  cubemap_shader.use();
  cubemap_shader.set<glm::mat4>("projection", projection);
  cubemap_shader.set<glm::mat4>("view", view_no_translation);
  glDepthFunc(GL_LEQUAL);
  cubemap_texture.use(0);
  skybox.render();
  glDepthFunc(GL_LESS);

  if (selected_satellite != 0)
    ui.render(satellites[selected_satellite]);
}
