#include <cmath>
#include <tuple>

const double KE = 0; // sqrt(G * M_earth)
const double AE = 0; // Equatorial radius of the Earth
const double J2 = 0; // Second gravitational zonal harmonic of the Earth
const double J3 = 0; // Third gravitational zonal harmonic of the Earth
const double ER = 0; // Radius of the Earth (km)

// clang-format off
/**
 * The TLE stores data that has already been averaged in specific ways.
 * Thus it's necessary to process the mean motion to recover its true value.
 *  @return <True mean motion, True semi major axis>
 *
 * The following paramers are to be supplied from the TLE data.
 * @param n0 Mean motion of orbit (defined as 2 * PI / T)
 * @param i0 Inclination
 * @param e0 Eccentricity
 */
std::tuple<double, double> recover_true_elements(double n0, double i0,
                                                 double e0) {
  const double ti = cos(i0);
  // Rearrange Kepler's third law to compute a first-guess semi major axis
  double a1 = std::pow((KE / n0), 2.0 / 3.0);
  // Compute a first order correction due to the Earth's oblateness
  double k2 = 0.5 * J2 * AE * AE;
  double d1 = (3.0 * k2 * (3.0 * ti * ti - 1.0)) / (2.0 * a1 * a1 * pow((1.0 - e0 * e0), 3.0 / 2.0));
  // Refine the semi major axis and correction
  double a0 = a1 * (1.0 - (1.0 / 3.0) * d1 - d1 * d1 - (134.0 / 81.0) * d1 * d1 * d1);
  double d0 = (3.0 * k2 * (3.0 * ti * ti - 1.0)) / (2.0 * a0 * a1 * pow((1.0 - e0 * e0), 3.0 / 2.0));
  // Compute the true mean motion and true semi major axis
  return {n0 / (1.0 + d0), a0 / (1.0 - d0)};
}

/**
 * @param drag Drag
 * @param o0 Argument of perigee
 */
void compute_constants(double n0, double i0, double e0, double drag,
                       double o0, double perigee, double q0) {
  const double ti = cos(i0);
  const double a30 = -J3 * AE * AE * AE;
  double k2 = 0.5 * J2 * AE * AE;

  auto [n, a] = recover_true_elements(n0, i0, e0);

  // Adjust the reference altitude (s) and scale parameter (st) based off of perigee
  double s = 1.01222928;
  double st = pow((q0 - s), 4);

  if (perigee >= 98.0 && perigee <= 156.0) {
    double new_s = s = a * (1.0 - e0) - s + AE;
    st = pow((q0 - s) + s - new_s, 4.0);
    s = new_s;
  } else if (perigee < 98.0) {
    double new_s = 20.0 / 6378.135 + AE;
    st = pow((q0 - s) + s - new_s, 4.0);
    s = new_s;
  }

  // Compute constants
  double ep = 1.0 / (a - s);
  double ep4 = ep * ep * ep * ep;

  double b0 = pow(1.0 - e0 * e0, 1.0 / 2.0);
  double nu = a * e0 * ep;
  double nu4 = nu * nu * nu * nu;
  double h = pow((1.0 - nu * nu), -3.5);
  double m =  (2.0 * st * ep4 * a * b0 * b0 * h);

  // Central drag coefficient
  double c2 = st * ep4 * n * h;
  c2 *= (a * (1.0 + 1.5 * nu * nu + 4.0 * e0 * nu + e0 * nu * nu * nu) +
        ((1.5 * k2 * ep) / (1.0 - nu * nu)) * (-0.5 + 1.5 * ti * ti) *
        (8.0 + 24 * nu * nu + 3.0 * nu4));

  double c1 = drag * c2; // Fundamental drag rate

  // Higher order drag terms
  double c3 = (st * ep4 * ep * a30 * n * AE * sin(i0)) / (k2 * e0);
  double c4 = n * m *
      ((2.0 * nu * (1.0 + e0 * nu) + 0.5 * e0 + 0.5 * nu * nu * nu) -
      ((2.0 * k2 * ep) / (a * (1.0 - nu * nu))) *
      ((3.0 - 9.0 * ti * ti) * (1.0 + 1.5 * nu * nu - 2.0 * e0 * nu - 0.5 * e0 * nu * nu * nu) +
      (0.75 - 0.75 * ti * ti) * (2.0 * nu * nu - e0 * nu - e0 * nu * nu * nu) * cos(2.0 * o0)));
  double c5 = m * (1.0 + (11.0 / 4.0) * nu * (nu + e0) + e0 * nu * nu * nu);

  // Time power coefficients for the semimajor axis decay
  double d2 = 4.0 * a * ep * c1 * c1;
  double d3 = (4.0 / 3.0) * a * ep * ep * (17 * a + s) * c1 * c1 * c1;
  double d4 = (2.0 / 3.0) * a * ep * ep * ep * (221.0 * a + 31.0 * s) * c1 * c1 * c1 * c1;

  // TODO: p.11
}

// clang-format on