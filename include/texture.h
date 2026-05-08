#pragma once

#include <string>
#include <vector>

class Texture {
public:
  ~Texture();
  void init(std::vector<std::string> paths);
  void use(int unit);

private:
  unsigned int id, obj;
};

class Framebuffer {
public:
  ~Framebuffer();
  Framebuffer() : initialized(false) {}

  void clear();
  void bind(bool use);
  void resize(int width, int height);
  unsigned int read_value(int x, int y);

private:
  bool initialized;
  unsigned int fbo, rbo, texture;
};
