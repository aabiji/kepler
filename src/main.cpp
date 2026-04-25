#include <assert.h>
#include <fstream>
#include <iostream>
#include <vector>

#include <perturb/perturb.hpp>
#include <perturb/tle.hpp>

#include <glm/glm.hpp>

struct Satellite {
  std::string name;
  std::string id;
  perturb::Satellite model;
  glm::vec3 position; // km
  glm::vec3 velocity; // km/s

  Satellite(std::string name, std::string id, perturb::TwoLineElement info);
  void propagate(int minutes_since_epoch);
};

Satellite::Satellite(std::string name, std::string id,
                     perturb::TwoLineElement info)
    : name(name), id(id), model(perturb::Satellite(info)) {
  assert(model.last_error() == perturb::Sgp4Error::NONE);
}

void Satellite::propagate(int minutes_since_epoch) {
  perturb::StateVector s;
  auto ret = model.propagate_from_epoch(minutes_since_epoch, s);
  assert(ret == perturb::Sgp4Error::NONE);
  position = glm::vec3(s.position[0], s.position[1], s.position[2]);
  velocity = glm::vec3(s.velocity[0], s.velocity[1], s.velocity[2]);
}

double epoch_day_of_year(std::string timestamp) {
  // Parse the ISO8601 string
  int year = std::stoi(timestamp.substr(2, 2)); // Last two digits
  int month = std::stoi(timestamp.substr(5, 2));
  int day = std::stoi(timestamp.substr(8, 2));
  int hour = std::stoi(timestamp.substr(11, 2));
  int minute = std::stoi(timestamp.substr(14, 2));
  int second = std::stoi(timestamp.substr(17, 2));
  double fractional_second =
      std::stoi(timestamp.substr(20, timestamp.length() - 20));

  // Accumulate days
  int month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((year % 4 == 0) && (year % 100 != 0 || year % 400 == 0))
    month_days[1] = 29; // Leap year

  double days = day;
  for (int m = 0; m < month; m++)
    days += month_days[m];

  // Add the day fraction
  double total_seconds =
      hour * 3600.0 + minute * 60.0 + second + fractional_second;
  return days + total_seconds / 86400.0;
}

std::vector<Satellite> read_satellite_data(const char *csv_path) {
  std::ifstream file(csv_path);
  assert(file.good() && file.is_open());

  std::vector<Satellite> satellites;

  bool column_line = true;
  std::string line = "";
  while (std::getline(file, line)) {
    if (column_line) {
      column_line = false;
      continue;
    }

    perturb::TwoLineElement info;
    std::string name = "", id = "";
    size_t current = 0, column = 0;

    while (current < line.length()) {
      size_t next = std::min(line.find(",", current), line.length());
      std::string str = line.substr(current, next - current);
      current = next + 1;
      column++;

      // clang-format off
      switch (column - 1) {
        case 0: name = str; break;
        case 1: id = str; break;
        case 2: {
          int year = std::stoi(str.substr(2, 2)); // Last two digits
          info.epoch_day_of_year = epoch_day_of_year(str);
          info.launch_year = year;
          info.epoch_year = year;
          break;
        }
        case 3: info.mean_motion = std::stod(str); break;
        case 4: info.eccentricity = std::stod(str); break;
        case 5: info.inclination = std::stod(str); break;
        case 6: info.raan = std::stod(str); break;
        case 7: info.arg_of_perigee = std::stod(str); break;
        case 8: info.mean_anomaly = std::stod(str); break;
        case 9: info.ephemeris_type = std::stol(str); break;
        case 10: info.classification = str[0]; break;
        case 11: {
          info.catalog_number[0] = str[0];
          info.catalog_number[1] = str[1];
          info.catalog_number[2] = str[2];
          info.catalog_number[3] = str[3];
          info.catalog_number[4] = str[4];
          info.catalog_number[5] = str.length() == 6 ? str[5] : ' ';
          break;
        }
        case 12: info.element_set_number = std::stoi(str); break;
        case 13: info.revolution_number = std::stol(str); break;
        case 14: info.b_star = std::stod(str); break;
        case 15: info.n_dot = std::stod(str); break;
        case 16: info.n_ddot = std::stod(str); break;
      };
      // clang-format on
    }

    satellites.push_back(Satellite(name, id, info));
  }

  file.close();
  return satellites;
}

int main() {
  const char *path = "../data/starlink.csv";
  auto satellites = read_satellite_data(path);
  for (auto s : satellites) {
    s.propagate(5);
    std::cout << "Position: " << s.position.x << ", " << s.position.y << ", "
              << s.position.z << "\n";
    std::cout << "Velocity: " << s.velocity.x << ", " << s.velocity.y << ", "
              << s.velocity.z << "\n";
  }
  return 0;
}