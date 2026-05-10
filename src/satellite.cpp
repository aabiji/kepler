#include <chrono>
#include <ctime>
#include <fstream>
#include <glm/gtc/matrix_access.hpp>
#include <httplib.h>
#include <perturb/tle.hpp>
#include <string_view>
#include <utility>

#include "misc.h"
#include "satellite.h"

const double DAY_SECONDS = 86400.0;
using sysclock = std::chrono::system_clock;

std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  if (first == std::string::npos)
    return ""; // String is all whitespace
  size_t last = str.find_last_not_of(" \t\n\r");
  return str.substr(first, (last - first + 1));
}

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

std::string tle_epoch_to_utc(int epoch_year, double epoch_day) {
  // TLE convention
  int year = (epoch_year >= 57) ? (1900 + epoch_year) : (2000 + epoch_year);
  int whole_days = static_cast<int>(epoch_day);
  double fractional_day = epoch_day - whole_days;
  int seconds = static_cast<int>(fractional_day * 86400.0);

  std::tm tm = {};
  tm.tm_year = year - 1900;
  tm.tm_mon = 0;
  tm.tm_mday = 1;

  // Convert Jan 1 UTC to timestamp and add day-of-year offset
  std::time_t t = timegm(&tm) + (whole_days - 1) * 86400 + seconds;
  struct std::tm utc;
  gmtime_r(&t, &utc);

  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S UTC", &utc);
  return buffer;
}

std::string fetch_tle_data(std::string cache_path) {
  auto duration = sysclock::now().time_since_epoch();
  auto seconds =
      std::chrono::duration_cast<std::chrono::seconds>(duration).count();

  // Prefer to read the cached data
  std::ifstream infile(cache_path);
  if (infile.is_open()) {
    std::string first_line;
    std::getline(infile, first_line);

    int64_t timestamp = std::stoll(first_line);
    double elapsed_hours = double(seconds - timestamp) / 3600.0;
    bool need_refresh = elapsed_hours >= 2.0;

    if (!need_refresh) { // Read the rest of the file
      std::stringstream buffer;
      buffer << infile.rdbuf();
      return buffer.str();
    }
  }

  // Here, there was an error reading the file, or a refresh is needed
  httplib::Client client("https://celestrak.org");
  auto response = client.Get("/NORAD/elements/gp.php?GROUP=ACTIVE&FORMAT=TLE");
  if (!response || response->status != 200)
    THROW_ERROR("Failed to fetch");

  std::ofstream outfile(cache_path, std::ios::trunc);
  outfile << seconds << "\n" << response->body;
  return response->body;
}

double parse_tle_exp(std::string s) {
  s.erase(remove(s.begin(), s.end(), ' '), s.end());
  std::string mantissa = s.substr(0, s.size() - 2);
  std::string exponent = s.substr(s.size() - 2);
  return std::stod("0." + mantissa + "e" + exponent);
}

std::vector<Satellite> load_satellite_data(std::string &str) {
  std::stringstream ss(str);
  std::string name, l1, l2;
  std::vector<Satellite> output;

  while (std::getline(ss, name) && std::getline(ss, l1) &&
         std::getline(ss, l2)) {
    perturb::TwoLineElement info;
    info.ephemeris_type = l1[62];
    info.epoch_year = std::stoi(l1.substr(18, 2));
    info.epoch_day_of_year = std::stod(l1.substr(20, 12));
    info.n_ddot = parse_tle_exp(l1.substr(44, 8));
    info.b_star = parse_tle_exp(l1.substr(53, 8));
    info.element_set_number = std::stoi(l1.substr(64, 4));
    info.inclination = std::stod(l2.substr(8, 8));
    info.raan = std::stod(l2.substr(17, 8));
    info.eccentricity = std::stod("0." + l2.substr(26, 7));
    info.arg_of_perigee = std::stod(l2.substr(34, 8));
    info.mean_anomaly = std::stod(l2.substr(43, 8));
    info.mean_motion = std::stod(l2.substr(52, 11));
    info.revolution_number = std::stoi(l2.substr(63, 5));

    std::string date =
        tle_epoch_to_utc(info.epoch_year, info.epoch_day_of_year);
    std::string norad_id = l1.substr(2, 5);

    auto model = perturb::Satellite(info);
    handle_error(model.last_error());
    output.push_back({trim(name), trim(norad_id), date, info.mean_motion,
                      info.inclination, info.eccentricity, model});
  }
  return output;
}

perturb::JulianDate get_current_time() {
  auto tt = sysclock::to_time_t(sysclock::now());
  struct std::tm utc_time;
  gmtime_r(&tt, &utc_time);
  auto time = perturb::DateTime{
      utc_time.tm_year + 1900, utc_time.tm_mon + 1, utc_time.tm_mday,
      utc_time.tm_hour,        utc_time.tm_min,     (double)utc_time.tm_sec};
  return perturb::JulianDate(time);
}

glm::vec3 propagate(perturb::Satellite model, perturb::JulianDate time) {
  perturb::StateVector s;
  handle_error(model.propagate(time, s));

  // Scale kilometers to on screen coordinates by dividing by the Earth's radius
  glm::vec3 position =
      glm::vec3(s.position[0], s.position[1], s.position[2]) * (1.0f / 6371.0f);
  // TEME defines the Z axis as pointing up, while we define the Y axis as pointing up
  return glm::vec3(position.x, position.z, position.y);
}

// Predict the full trajectory of a satellite given an initial configuration
std::vector<glm::vec3> compute_trajectory(Satellite satellite) {
  double period = DAY_SECONDS / satellite.mean_motion;
  perturb::JulianDate time = get_current_time();
  std::vector<glm::vec3> trajectory;
  double step = 30;
  for (double seconds = 0; seconds < period; seconds += step) {
    trajectory.push_back(propagate(satellite.model, time));
    time += step / DAY_SECONDS;
  }
  return trajectory;
}

void simulate_satellites(std::stop_token token,
                         const std::vector<Satellite> satellites,
                         SharedInstanceData &shared) {
  perturb::JulianDate simulation_time = get_current_time();

  std::time_t tt = sysclock::to_time_t(sysclock::now());
  struct std::tm target_time;
  localtime_r(&tt, &target_time);
  target_time.tm_sec++;

  auto step = [&]() {
    std::vector<InstanceData> temp;
    for (size_t i = 0; i < satellites.size(); i++) {
      glm::vec3 position = propagate(satellites[i].model, simulation_time);
      temp.push_back(InstanceData(position, glm::vec3(0.01, 0.01, 0.01), true));
    }

    std::lock_guard<std::mutex> guard(shared.mutex);
    std::swap(temp, shared.data);
  };

  step(); // Get initial positions
  while (!token.stop_requested()) {
    simulation_time += (1.0 / DAY_SECONDS);
    step();

    // Sleep until the next second
    std::time_t normalized = std::mktime(&target_time);
    std::this_thread::sleep_until(sysclock::from_time_t(normalized));
    target_time.tm_sec++;
  }
}
