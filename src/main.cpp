#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "debug.h"
#include "mesh.h"
#include "satellite.h"
#include "shader.h"

/*
Refactor checklist (the code right now is extremely messy):

- merge instancedmesh and skybox?
- don't need to store vertices + indices + instance data after they've been
uploaded to the shader
- shouldn't be using data.size() as the instance count
- resize opengl viewport
- properly distinguish camera rotation vs globe rotation
  rotate around the cube using left/right arrow keys, rotate around view using
mouse
- merge the two texture constructots into one
*/

class Camera {
public:
  explicit Camera(float aspect_ratio) {
    projection =
        glm::perspective((float)M_PI / 4.0f, aspect_ratio, 0.1f, 100.0f);
    pos = glm::vec3(0.0, 0.0, 3.0);
  }

  void rotate_look_at(float dx, float dy, float sensitivity) {
    glm::vec3 world_up = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 world_right = current_rotation * glm::vec3(1.0, 0.0, 0.0);

    glm::quat yaw = glm::angleAxis(dx * sensitivity, world_up);
    glm::quat pitch = glm::angleAxis(dy * sensitivity, world_right);
    current_rotation = glm::normalize(yaw * pitch * current_rotation);
  }

  glm::mat4 view_matrix() {
    glm::vec3 front = current_rotation * glm::vec3(0.0, 0.0, -1.0);
    glm::vec3 up = current_rotation * glm::vec3(0.0, 1.0, 0.0);
    return glm::lookAt(pos, pos + front, up);
  }

  glm::mat4 projection_matrix() { return projection; }

private:
  glm::vec3 pos;
  glm::quat current_rotation;
  glm::mat4 projection;
};

class Texture {
public:
  ~Texture() { glDeleteTextures(1, &id); }

  Texture(int id, int obj) : id(id), obj(obj) {}

  static Texture from_image(const char *path) {
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 3);
    if (pixels == nullptr || channels != 3)
      THROW_ERROR("Failed to load {}", path);

    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return Texture(id, GL_TEXTURE_2D);
  }

  // NOTE: the images must be in the following order: +x, -x, +y, -y, +z, -z
  static Texture from_cubemap(const char *paths[]) {
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    for (int i = 0; i < 6; i++) {
      int width = 0, height = 0, channels = 0;
      unsigned char *pixels =
          stbi_load(paths[i], &width, &height, &channels, 3);
      if (pixels == nullptr || channels != 3)
        THROW_ERROR("Failed to load {}", paths[i]);
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                   0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
      stbi_image_free(pixels);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    return Texture(id, GL_TEXTURE_CUBE_MAP);
  }

  void use() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(obj, id);
  }

private:
  unsigned int id, obj;
};

InstanceData satellite_to_model(Satellite s) {
  s.propagate(0);
  glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01));
  glm::mat4 translate = glm::translate(glm::mat4(1.0), s.position * 0.0001f);
  InstanceData instance;
  instance.model_matrix = translate * scale;
  instance.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
  return instance;
}

int main() {
  // auto satellites = read_satellite_data("../assets/starlink.csv");

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int window_width = 900, window_height = 700;
  GLFWwindow *window = glfwCreateWindow(window_width, window_height,
                                        "LEO Visualization", nullptr, nullptr);
  if (window == nullptr)
    THROW_ERROR("Failed to create window");

  glfwMakeContextCurrent(window);
  assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) != 0);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

  glViewport(0, 0, window_width, window_height);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(gl_debug_callback, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                        GL_TRUE);

  float aspect_ratio = (float)window_width / (float)window_height;
  Camera camera(aspect_ratio);
  const char *filenames[] = {
      "../assets/textures/cubemap/px.png", "../assets/textures/cubemap/nx.png",
      "../assets/textures/cubemap/py.png", "../assets/textures/cubemap/ny.png",
      "../assets/textures/cubemap/pz.png", "../assets/textures/cubemap/nz.png"};

  {
    Shader main_shader("../assets/shaders/main_vertex.glsl",
                       "../assets/shaders/main_fragment.glsl");

    Shader cubemap_shader("../assets/shaders/cubemap_vertex.glsl",
                          "../assets/shaders/cubemap_fragment.glsl");

    Texture cubemap_texture = Texture::from_cubemap(filenames);
    Texture earth_texture =
        Texture::from_image("../assets/textures/earth/daymap.jpg");

    InstancedMesh globe = create_unit_sphere(32, 32);
    InstanceData globe_instance;
    globe_instance.model_matrix = // Scale
        glm::scale(glm::mat4(1.0), glm::vec3(2.0, 2.0, 2.0));
    globe.data.push_back(globe_instance);
    globe.init_buffers();

    Skybox skybox;

    // std::transform(satellites.begin(), satellites.end(),
    //                std::back_inserter(circles.data), satellite_to_model);
    // InstancedMesh circles = create_circle_mesh(10);
    // circles.init_buffers();

    double prev_x = 0, prev_y = 0;
    glfwGetCursorPos(window, &prev_x, &prev_y);

    while (!glfwWindowShouldClose(window)) {
      if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        break;

      /*
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.move(-1);

      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.move(1);
      */

      double x, y;
      glfwGetCursorPos(window, &x, &y);
      bool mouse_down =
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      if (mouse_down)
        camera.rotate_look_at(x - prev_x, y - prev_y, 0.01);
      prev_x = x;
      prev_y = y;

      glClearColor(0.0, 0.0, 0.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 p = camera.projection_matrix();
      glm::mat4 v = camera.view_matrix();
      glm::mat4 v_no_translation = glm::mat4(glm::mat3(v));

      // Render the scene
      main_shader.use();
      main_shader.set<glm::mat4>("projection", p);
      main_shader.set<glm::mat4>("view", v);

      earth_texture.use();
      main_shader.set<bool>("use_texture", true);
      globe.render();

      // glDisable(GL_DEPTH_TEST); // For drawing the 2D shapes
      // main_shader.set<bool>("use_texture", false);
      // circles.render();
      // glEnable(GL_DEPTH_TEST);

      // Render the skybox
      cubemap_shader.use();
      cubemap_shader.set<glm::mat4>("projection", p);
      cubemap_shader.set<glm::mat4>("view", v_no_translation);
      glDepthFunc(GL_LEQUAL);
      cubemap_texture.use();
      skybox.render();
      glDepthFunc(GL_LESS);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  }

  glfwTerminate();
  return 0;
}
