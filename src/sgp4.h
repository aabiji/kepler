#pragma once

#include <string>

struct vec3 {
  double x, y, z;
  vec3(double a, double b, double c) : x(a), y(b), z(c) {}

  friend vec3 operator*(vec3 a, vec3 b) {
    return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
  }

  friend vec3 operator*(vec3 a, double b) {
    return vec3(a.x * b, a.y * b, a.z * b);
  }

  friend vec3 operator*(double a, vec3 b) {
    return vec3(a * b.x, a * b.y, a * b.z);
  }

  friend vec3 operator+(vec3 a, vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
  }

  friend vec3 operator-(vec3 a, vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
  }
};

// Column indexes
// https://www.amsat.org/keplerian-elements-tutorial/
// TODO: use an arena allocator to allocate strings
typedef struct {
  std::string name;
  std::string id;
  std::string epoch; // TODO: parse this date
  double mean_motion;
  double eccentricity;
  double inclination;
  double raan; // Right ascension of the ascending node
  double arg_of_perigee;
  double mean_anomaly;
  std::string ephemeris_type;
  std::string classification_type;
  int catalogue_id;
  double element_set_number;
  double revolutions_at_epoch;
  double bstar_drag;
  double mean_motion_deriv1;
  double mean_motion_deriv2;
  // The root mean squared error of the orbital fit indicates how well
  // the generated data matches the raw tracking data used to create it.
  double rms_error;
  std::string data_source;
} SatelliteInfo;

typedef struct {
  vec3 P, V;
} Prediction;

Prediction sgp4_propagate(SatelliteInfo info);