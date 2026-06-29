// Weighted linear least-squares trilateration.
//
// Used to compute an initial position estimate from accumulated anchor ranges
// before the position filter is initialised.  Not used during normal operation —
// the filter's updateRange() takes over once init() has been called.
//
// Algorithm:
//   Subtracting the last anchor's range equation from all others linearises the
//   otherwise quadratic system, yielding Ax = b.  Solving with WLS:
//     x_hat = (A^T W A)^{-1} A^T W b
//   where W = diag(1/σ_i²) weights down uncertain (low rx_power) measurements.
#pragma once

#include <Eigen/Dense>
#include <cmath>
#include <vector>

#include "anchor_map.h"

namespace uwb_positioning {

// Returns 1/σ² for one range observation given its received signal power.
// Uses the same noise model as the EKF so trilateration weights are consistent.
//   sigma_r      : base range std-dev (m) at rx_ref_dbm
//   rx_ref_dbm   : reference rx_power at which sigma_r applies
//   rx_decay_db  : signal drop (dB) that multiplies noise by 10×
inline double rxToWeight(double rx_power_dbm,
                         double rx_ref_dbm,
                         double rx_decay_db,
                         double sigma_r) {
  const double loss_db    = std::max(0.0, rx_ref_dbm - rx_power_dbm);
  const double noise_gain = std::pow(10.0, loss_db / rx_decay_db);
  const double sigma      = sigma_r * noise_gain;
  return 1.0 / (sigma * sigma);
}

// Weighted LS trilateration.
//   anchors : >= 3 entries with known 3-D positions
//   ranges  : measured distances, same order as anchors
//   weights : 1/σ_i² per entry; pass empty vector for unit weights
// Returns true and writes (x_out, y_out, z_out) on success.
// Returns false if the geometry is degenerate (det ≈ 0).
inline bool trilaterate(const std::vector<Anchor>& anchors,
                        const std::vector<double>&  ranges,
                        const std::vector<double>&  weights,
                        double& x_out, double& y_out, double& z_out) {
  const int M = static_cast<int>(anchors.size());
  if (M < 3 || static_cast<int>(ranges.size()) != M) return false;

  // Reference anchor (last entry): subtract its equation from the others.
  const Anchor& aN  = anchors[M - 1];
  const double  rN  = ranges[M - 1];
  const double  rN2 = rN * rN;
  const double  dN2 = aN.x*aN.x + aN.y*aN.y + aN.z*aN.z;

  Eigen::MatrixXd A(M - 1, 3);
  Eigen::VectorXd b(M - 1), w(M - 1);
  for (int i = 0; i < M - 1; ++i) {
    const Anchor& ai = anchors[i];
    const double  ri = ranges[i];
    A(i, 0) = 2.0 * (aN.x - ai.x);
    A(i, 1) = 2.0 * (aN.y - ai.y);
    A(i, 2) = 2.0 * (aN.z - ai.z);
    b(i)    = ri*ri - rN2 - (ai.x*ai.x + ai.y*ai.y + ai.z*ai.z) + dN2;
    w(i)    = weights.empty() ? 1.0 : weights[i];
  }

  const Eigen::MatrixXd W    = w.asDiagonal();
  const Eigen::MatrixXd AtWA = A.transpose() * W * A;
  if (std::abs(AtWA.determinant()) < 1e-10) return false;

  const Eigen::Vector3d sol = AtWA.ldlt().solve(A.transpose() * W * b);
  x_out = sol(0); y_out = sol(1); z_out = sol(2);
  return true;
}

}  // namespace uwb_positioning
