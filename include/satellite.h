#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <perturb/perturb.hpp>
#include <perturb/tle.hpp>

struct Satellite {
  std::string name;
  std::string id;
  perturb::Satellite model;
  glm::vec3 position; // km
  glm::vec3 velocity; // km/s

  Satellite(std::string name, std::string id, perturb::TwoLineElement info);
  void propagate(int minutes_since_epoch);
};

std::vector<Satellite> read_satellite_data(const char *csv_path);