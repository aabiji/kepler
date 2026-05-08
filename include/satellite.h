#pragma once

#include <mutex>
#include <stop_token>
#include <vector>

#include "mesh.h"

struct SharedSatelliteInfo {
  std::vector<InstanceData> data;
  std::mutex mutex;
};

void simulate_satellites(std::stop_token token, const char *input_csv_path,
                         SharedSatelliteInfo &shared);
