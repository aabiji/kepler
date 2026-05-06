#pragma once

#include <stop_token>

#include "mesh.h"

void simulate_satellites(std::stop_token token, const char *input_csv_path,
                         SharedInstances &shared);
