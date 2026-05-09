#include <filesystem>
#include <glad/glad.h>
#include <iostream>

#include "misc.h"

void gl_debug_callback(GLenum src, GLenum type, unsigned int id,
                       GLenum severity, GLsizei _length, const char *message,
                       const void *_user_param) {
  // Ignore insignificant error/warning codes
  // clang-format off
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
  (void)_length;
  (void)_user_param;

  std::cout << "---------------" << std::endl;
  std::cout << "Debug message (" << id << "): " <<  message << std::endl;

  switch (src)
  {
      case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
      case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
      case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
      case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
      case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
  } std::cout << std::endl;

  switch (type)
  {
      case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
      case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
      case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
      case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
      case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
      case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
      case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
  } std::cout << std::endl;

  switch (severity)
  {
      case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
      case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
      case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
      case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
  } std::cout << std::endl;
  std::cout << std::endl;
  // clang-format on
}

std::string verify(std::string path) {
  bool file_exists = std::filesystem::exists(path);
  if (!file_exists)
    THROW_ERROR("{} not found", path);
  return path;
}

std::string font_path() { return verify("assets/Roboto-Regular.ttf"); }

std::vector<std::string> cubemap_texture_paths() {
  return {verify("assets/cubemap/px.png"), verify("assets/cubemap/nx.png"),
          verify("assets/cubemap/py.png"), verify("assets/cubemap/ny.png"),
          verify("assets/cubemap/pz.png"), verify("assets/cubemap/nz.png")};
}

std::vector<std::string> earth_texture_paths() {
  return {verify("assets/earth/texture.png"), verify("assets/earth/normal.png"),
          verify("assets/earth/specular.png")};
}

std::vector<std::string> shader_paths() {
  return {verify("assets/shaders/vmain.glsl"),
          verify("assets/shaders/fmain.glsl"),
          verify("assets/shaders/vbuffer.glsl"),
          verify("assets/shaders/fbuffer.glsl"),
          verify("assets/shaders/vcubemap.glsl"),
          verify("assets/shaders/fcubemap.glsl")};
}
