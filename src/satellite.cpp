#include <fstream>

#include "debug.h"
#include "satellite.h"

void handle_error(perturb::Sgp4Error err) {
  std::string msg = "";

  // clang-format off
  switch (err) {
    case perturb::Sgp4Error::MEAN_ELEMENTS: msg = "Bad mean elements"; break;
    case perturb::Sgp4Error::MEAN_MOTION: msg = "Bad mean motion"; break;
    case perturb::Sgp4Error::PERT_ELEMENTS: msg = "Bad pertubation elements"; break;
    case perturb::Sgp4Error::SEMI_LATUS_RECTUM: msg = "Bad semi latus rectum"; break;
    case perturb::Sgp4Error::EPOCH_ELEMENTS_SUB_ORBITAL: msg = "Bad sub orbital epoch elements"; break;
    case perturb::Sgp4Error::DECAYED: msg = "Decayed prediction"; break;
    case perturb::Sgp4Error::INVALID_TLE: msg = "Invalid TLE"; break;
    default: msg = "Uknown sgp4 error"; break;
  };
  // clang-format on

  if (err != perturb::Sgp4Error::NONE)
    THROW_ERROR("ERROR: {}", msg);
}

Satellite::Satellite(std::string name, std::string id,
                     perturb::TwoLineElement info)
    : name(name), id(id), model(perturb::Satellite(info)) {
  handle_error(model.last_error());
}

void Satellite::propagate(int minutes_since_epoch) {
  perturb::StateVector s;
  handle_error(model.propagate_from_epoch(minutes_since_epoch, s));
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
  if (!file.good() || !file.is_open())
    THROW_ERROR("Failed to open {}", csv_path);

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