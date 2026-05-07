#pragma once

#include <string>

class Shader {
public:
  ~Shader();

  void use();
  void init(std::string vertex_shader, std::string fragment_shader);
  template <typename T> void set(const char *name, T value);

private:
  unsigned int program;
};
