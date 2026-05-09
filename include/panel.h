#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "satellite.h"

struct InfoPanel {
  ~InfoPanel();
  void init(GLFWwindow *window);
  bool render(Satellite *satellite, std::string error);
  bool active();
  std::string search_term;
};
