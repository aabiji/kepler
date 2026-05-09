#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "satellite.h"

struct InfoUI {
  ~InfoUI();
  void init(GLFWwindow *window);
  void render(Satellite satellite);
  bool active();
};
