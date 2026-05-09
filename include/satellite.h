#pragma once

#include <mutex>
#include <perturb/perturb.hpp>
#include <stop_token>

#include "mesh.h"

struct Satellite {
  std::string name;
  std::string norad_id;
  std::string epoch;
  double mean_motion;
  double inclination;
  double eccentricity;
  perturb::Satellite model;
};

struct SharedInstanceData {
  std::vector<InstanceData> data;
  std::mutex mutex;
};

std::string fetch_tle_data(std::string cache_path);
std::vector<Satellite> load_satellite_data(std::string &str);

std::vector<glm::mat3> compute_trajectory(Satellite satellite);
void simulate_satellites(std::stop_token token,
                         const std::vector<Satellite> satellites,
                         SharedInstanceData &shared);
