#pragma once

class Shader {
public:
  explicit Shader(const char *vshader_path, const char *fshader_path);
  ~Shader();

  void use();
  template <typename T> void set(const char *name, T value);

private:
  unsigned int program;
};