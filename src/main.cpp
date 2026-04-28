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

glm::quat rotate(glm::quat rotation, float padx, float pady, float dx,
                 float dy) {
  glm::vec3 world_up = glm::vec3(0.0, 1.0, 0.0);
  glm::vec3 world_right = rotation * glm::vec3(1.0, 0.0, 0.0);

  glm::quat yaw = glm::angleAxis(dx * padx, world_up);
  glm::quat pitch = glm::angleAxis(dy * pady, world_right);

  return glm::normalize(yaw * pitch * rotation);
}

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
  auto satellites = read_satellite_data("../data/starlink.csv");

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

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(gl_debug_callback, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                        GL_TRUE);

  float aspect_ratio = (float)window_width / (float)window_height;
  Camera camera(aspect_ratio);

  {
    Shader shader("../data/vertex.glsl", "../data/fragment.glsl");

    InstancedMesh globe = generate_unit_sphere(32, 32);
    globe.init_buffers(1);

    InstancedMesh circles = generate_circle_mesh(10);
    circles.init_buffers(satellites.size());

    glm::quat globe_rotation = glm::quat(1.0, 0.0, 0.0, 0.0);
    glm::mat4 globe_scale =
        glm::scale(glm::mat4(1.0), glm::vec3(2.0, 2.0, 2.0));
    globe.data.push_back(InstanceData());

    std::transform(satellites.begin(), satellites.end(),
                   std::back_inserter(circles.data), satellite_to_model);

    const char *path = "../data/8081_earthmap4k.jpg";
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
      if (mouse_down) {
        globe_rotation =
            rotate(globe_rotation, 0.01, 0.0025, x - prev_x, y - prev_y);
        globe.data[0].model_matrix = glm::mat4(globe_rotation) * globe_scale;
      }
      prev_x = x;
      prev_y = y;

      glClearColor(0.0, 0.0, 0.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 p = camera.projection_matrix();
      glm::mat4 v = camera.view_matrix();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);

      shader.use();
      shader.set<glm::mat4>("projection", p);
      shader.set<glm::mat4>("view", v);

      glEnable(GL_DEPTH_TEST);
      shader.set<bool>("use_texture", true);
      globe.render();

      glDisable(GL_DEPTH_TEST);
      shader.set<bool>("use_texture", false);
      circles.render();

      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    glDeleteTextures(1, &texture);
  }

  glfwTerminate();
  return 0;
}
