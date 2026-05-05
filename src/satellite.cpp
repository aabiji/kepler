#include <chrono>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <perturb/perturb.hpp>
#include <perturb/tle.hpp>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

#include "debug.h"
#include "mesh.h"

const double DAY_SECONDS = 86400.0;
using sysclock = std::chrono::system_clock;

void handle_error(perturb::Sgp4Error err) {
  std::string msg = "";

  // clang-format off
  switch (err) {
    case perturb::Sgp4Error::MEAN_ELEMENTS: msg = "Bad mean elements"; break;
    case perturb::Sgp4Error::MEAN_MOTION: msg = "Bad mean motion"; break;
    case perturb::Sgp4Error::PERT_ELEMENTS: msg = "Bad perturbation elements"; break;
    case perturb::Sgp4Error::SEMI_LATUS_RECTUM: msg = "Bad semi latus rectum"; break;
    case perturb::Sgp4Error::EPOCH_ELEMENTS_SUB_ORBITAL: msg = "Bad sub orbital epoch elements"; break;
    case perturb::Sgp4Error::DECAYED: msg = "Decayed prediction"; break;
    case perturb::Sgp4Error::INVALID_TLE: msg = "Invalid TLE"; break;
    default: msg = "Unknown sgp4 error"; break;
  };
  // clang-format on

  if (err != perturb::Sgp4Error::NONE)
    THROW_ERROR("ERROR: {}", msg);
}

double epoch_day_of_year(std::string timestamp) {
  // Parse the ISO8601 string
  // And, yes this TLE convention is diabolically cursed.
  int year = std::stoi(timestamp.substr(2, 2)); // Last two digits
  int full_year = year >= 57 ? 1900 + year : 2000 + year;

  int month = std::stoi(timestamp.substr(5, 2));
  int day = std::stoi(timestamp.substr(8, 2));
  int hour = std::stoi(timestamp.substr(11, 2));
  int minute = std::stoi(timestamp.substr(14, 2));
  int second = std::stoi(timestamp.substr(17, 2));
  double fractional_second = std::stod("0." + timestamp.substr(20));

  // Accumulate days
  int month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((full_year % 4 == 0) && (full_year % 100 != 0 || full_year % 400 == 0))
    month_days[1] = 29; // Leap year

  double day_of_year = day;
  for (int m = 0; m < month - 1; m++)
    day_of_year += month_days[m];

  // Add the day fraction
  double elapsed_seconds =
      hour * 3600.0 + minute * 60.0 + second + fractional_second;
  return day_of_year + elapsed_seconds / DAY_SECONDS;
}

// Calculate the Greenwich Mean Sidereal Time, which is the angle between
// vernal equinox and the Earth's prime meridean, in radians.
double gmst_time(perturb::JulianDate date) {
  double full_date = date.jd + date.jd_frac;

  // Julian centuries since the J2000 epoch
  double jc = (full_date - 2451545.0) / 36525.0;

  // Calculate GMST in seconds using the IAU 1982 polynomial formula
  double seconds = 67310.54841 + (876600.0 * 3600.0 + 8640184.812866) * jc +
                   0.093104 * jc * jc - 6.2e-6 * jc * jc * jc;

  // Convert to radians [0, 2π]
  double radians = (seconds * 2.0 * M_PI) / DAY_SECONDS;
  double normalized = std::fmod(radians, (2.0 * M_PI));
  return normalized < 0.0 ? normalized + 2.0 * M_PI : normalized;
}

// Convert a position in the TEME reference from to the ITRS reference frame
// since positions in the TEME reference frame are tied to Earth's orientation
// in space at a specific time and do not rotate with the Earth.
glm::vec3 teme_to_itrs(glm::vec3 position, perturb::JulianDate date) {
  double angle = gmst_time(date);
  // Rotates vectors in the XY plane about the Z axis
  double sin_a = std::sin(angle), cos_a = std::cos(angle);
  glm::mat3 rotation =
      glm::mat3(cos_a, -sin_a, 0.0, sin_a, cos_a, 0.0, 0.0, 0.0, 1.0);
  return rotation * position;
}

struct Satellite {
  std::string name;
  std::string id;
  perturb::Satellite model;
};

class Constellation {
public:
  void set_current_time();
  int load_satellite_data(std::string csv_path);
  void propagate(double step_seconds, double earth_scale,
                 std::vector<InstanceData> &instances);

private:
  perturb::JulianDate date;
  std::vector<Satellite> satellites;
};

void Constellation::set_current_time() {
  // Set the current julian date
  auto tt = sysclock::to_time_t(sysclock::now());
  auto utc_time = *gmtime(&tt);
  auto time = perturb::DateTime{
      utc_time.tm_year + 1900, utc_time.tm_mon + 1, utc_time.tm_mday,
      utc_time.tm_hour,        utc_time.tm_min,     (double)utc_time.tm_sec};
  date = perturb::JulianDate(time);
}

int Constellation::load_satellite_data(std::string csv_path) {
  std::ifstream file(csv_path.c_str());
  if (!file.good() || !file.is_open())
    THROW_ERROR("Failed to open {}", csv_path);

  bool column_line = true;
  std::string line = "";
  satellites.clear();

  while (std::getline(file, line)) {
    if (column_line) {
      column_line = false;
      continue;
    }

    perturb::TwoLineElement info;
    std::string name = "", id = "";
    size_t current = 0, column = 0;

    // NOTE: the comma seperated values are expected to be in the order that
    // Celestrak defines
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
          std::memset(info.catalog_number, ' ', 6);
          for (size_t i = 0; i < str.length(); i++) {
            info.catalog_number[i] = str[i];
          }
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

    auto model = perturb::Satellite(info);
    handle_error(model.last_error());
    satellites.push_back({name, id, model});
  }

  file.close();
  return satellites.size();
}

void Constellation::propagate(double step_seconds, double earth_scale,
                              std::vector<InstanceData> &instances) {
  if (instances.size() != satellites.size())
    instances.resize(satellites.size());

  date += (step_seconds / DAY_SECONDS);
  for (size_t i = 0; i < satellites.size(); i++) {
    perturb::StateVector s;
    handle_error(satellites[i].model.propagate(date, s));

    glm::vec3 position = glm::vec3(s.position[0], s.position[1], s.position[2]);
    position = teme_to_itrs(position, date);
    // Scale kilometers to on screen coordinates by dividing by the Earth's radius
    position *= earth_scale / 6371.0;
    // ITRS defines the Z axis as pointing up, while we define the Y axis as pointing up
    position = glm::vec3(position.x, position.z, position.y);

    // Since the position is in the ITRS reference frame, and Earth is centered
    // at the origin, positions can directly be mapped to on screen positions
    instances[i] = InstanceData(position, glm::vec3(0.01, 0.01, 0.01));
    instances[i].color = glm::vec4(0.0, 1.0, 0.0, 1.0);
    instances[i].is_2d = true;
  }
}

void simulate_satellites(std::stop_token token, const char *input_csv_path,
                         std::vector<InstanceData> &data) {
  Constellation c;
  std::vector<InstanceData> back_buffer;

  // Initialize
  c.load_satellite_data(input_csv_path);
  c.set_current_time();

  // Compute initial positions
  c.propagate(0, 2.0, back_buffer);
  std::swap(data, back_buffer);

  // Propagate every 1 second
  std::time_t tt = sysclock::to_time_t(sysclock::now());
  struct std::tm *target_time = std::localtime(&tt);
  target_time->tm_sec++;

  while (!token.stop_requested()) {
    c.propagate(1, 2.0, back_buffer);
    std::swap(data, back_buffer);

    // Sleep until the next second
    std::time_t normalized = std::mktime(target_time);
    std::this_thread::sleep_until(sysclock::from_time_t(normalized));
    target_time->tm_sec++;
  }
}
