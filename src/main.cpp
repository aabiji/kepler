#include <fstream>
#include <iostream>
#include <vector>

#include "sgp4.h"

Prediction test_outputs[][] = {
    {{.P = vec3(2328.96594238, -5995.21600342, 1719.97894287),
      .V = vec3(2.91110113, -0.98164053, -7.09049922)},
     {.P = vec3(2456.00610352, -6071.94232177, 1222.95977784),
      .V = vec3(2.67852119, -0.44705850, -7.22800565)},
     {.P = vec3(2567.39477539, -6112.49725342, 713.97710419),
      .V = vec3(2.43952477, 0.09884824, -7.31889641)},
     {.P = vec3(2663.03179932, -6115.37414551, 195.73919105),
      .V = vec3(2.19531813, 0.65333930, -7.36169147)},
     {.P = vec3(2742.85470581, -6079.13580322, -328.86091614),
      .V = vec3(1.94707947, 1.21346101, -7.35499924)}},

    {{.P = vec3(2328.97048951, -5995.22076416, 1719.97067261),
      .V = vec3(2.91207230, -0.98341546, -7.09081703)},
     {.P = vec3(2456.10705566, -6071.93853760, 1222.89727783),
      .V = vec3(2.67938992, -0.44829041, -7.22879231)},
     {.P = vec3(2567.56195068, -6112.50384522, 713.96397400),
      .V = vec3(2.44024599, 0.09810869, -7.31995916)},
     {.P = vec3(2663.09078980, -6115.48229980, 196.39640427),
      .V = vec3(2.19611958, 0.65241995, -7.36282432)},
     {.P = vec3(2742.55133057, -6079.67144775, -326.38095856),
      .V = vec3(1.94850229, 1.21106251, -7.35619372)}},

    {{.P = vec3(7473.37066650, 428.95261765, 5828.74786377),
      .V = vec3(5.10715413, 6.44468284, -0.18613096)},
     {.P = vec3(-3305.22537232, 32410.86328125, -24697.17675781),
      .V = vec3(-1.30113538, -1.15131518, -0.28333528)},
     {.P = vec3(14271.28759766, 24110.46411133, -4725.76837158),
      .V = vec3(-0.32050445, 2.67984074, -2.08405289)},
     {.P = vec3(-9990.05883789, 22717.35522461, -23616.89062501),
      .V = vec3(-1.01667246, -2.29026759, 0.72892364)},
     {.P = vec3(9787.86975097, 33753.34667969, -15030.81176758),
      .V = vec3(-1.09425066, 0.92358845, -1.52230928)}}};

SatelliteInfo test_inputs[] = {
    (SatelliteInfo){
        .mean_motion = 16.05824518,
        .eccentricity = 0.0086731,
        .inclination = 72.8435,
        .raan = 115.9689,
        .arg_of_perigee = 52.6988,
        .mean_anomaly = 110.5714,
        .revolutions_at_epoch = 105,
        .bstar_drag = 6.6816e-05,
        .mean_motion_deriv1 = 0.00073094,
        .mean_motion_deriv2 = 1.3844e-04,
        .epoch_time = 80275.98708465,
    },
    (SatelliteInfo){
        .mean_motion = 16.05824518,
        .eccentricity = 0.0086731,
        .inclination = 72.8435,
        .raan = 115.9689,
        .arg_of_perigee = 52.6988,
        .mean_anomaly = 110.5714,
        .revolutions_at_epoch = 105,
        .bstar_drag = 6.6816e-05,
        .mean_motion_deriv1 = 0.00073094,
        .mean_motion_deriv2 = 1.3844e-04,
    },
    (SatelliteInfo){
        .mean_motion = 2.28537848,
        .eccentricity = 0.7318036,
        .inclination = 46.7916,
        .raan = 230.4354,
        .arg_of_perigee = 47.4722,
        .mean_anomaly = 10.4117,
        .catalogue_id = 11801,
        .element_set_number = 0,
        .revolutions_at_epoch = 0,
        .bstar_drag = 1.4311e-02,
        .mean_motion_deriv1 = 0.01431103,
        .mean_motion_deriv2 = 0.0,
    },
};

void test_spg4() {
  int num_cases = 3;
  for (int j = 0; j < num_cases; j++) {
    for (int i = 0; i <= 1440; i += 360) {
      Prediction value = sgp4_propagate(test_inputs[i]);
      assert(value.P == test_outputs[i].P);
      assert(value.V == test_outputs[i].V);
    }
  }
}

int read_satellite_csv(const char *path, std::vector<SatelliteInfo> &infos) {
  std::ifstream file(path);
  if (!file.good() || !file.is_open()) {
    std::cout << "Failed to open " << path << "\n";
    return -1;
  }

  bool first_line = true;
  std::string line = "";
  while (std::getline(file, line)) {
    if (first_line) {
      first_line = false;
      continue;
    }

    SatelliteInfo info;
    size_t current = 0, column = 0;

    while (current < line.length()) {
      size_t next = std::min(line.find(",", current), line.length());
      std::string substr = line.substr(current, next - current);
      current = next + 1;
      column++;

      // clang-format off
      switch (column - 1) {
        case 0: info.name = substr; break;
        case 1: info.id = substr; break;
        case 2: info.epoch = substr; break;
        case 3: info.mean_motion = std::stod(substr); break;
        case 4: info.eccentricity = std::stod(substr); break;
        case 5: info.inclination = std::stod(substr); break;
        case 6: info.raan = std::stod(substr); break;
        case 7: info.arg_of_perigee = std::stod(substr); break;
        case 8: info.mean_anomaly = std::stod(substr); break;
        case 9: info.ephemeris_type = substr; break;
        case 10: info.classification_type = substr; break;
        case 11: info.catalogue_id = std::stoi(substr); break;
        case 12: info.element_set_number = std::stod(substr); break;
        case 13: info.revolutions_at_epoch = std::stod(substr); break;
        case 14: info.bstar_drag = std::stod(substr); break;
        case 15: info.mean_motion_deriv1 = std::stod(substr); break;
        case 16: info.mean_motion_deriv2 = std::stod(substr); break;
        case 17: info.rms_error = std::stod(substr); break;
        case 18: info.data_source = substr; break;
      };
      // clang-format on
    }

    infos.push_back(info);
  }

  file.close();
  return 0;
}

int main() {
  const char *path = "../data/starlink.csv";
  std::vector<SatelliteInfo> infos;
  read_satellite_csv(path, infos);
  return 0;
}
