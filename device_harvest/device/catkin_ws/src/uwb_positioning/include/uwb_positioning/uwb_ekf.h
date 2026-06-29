// UWB tightly-coupled EKF and weighted trilateration.
//
// "Tightly coupled" means the EKF observation is the raw range measurement
// h_i(x) = ||x_tag - x_anchor_i||, not a pre-computed position.  This lets
// the filter work correctly even when fewer than 3 anchors are visible.
//
// State:   [x, y, z, vx, vy, vz]^T  (6-dim)
// Process: constant-velocity model with white-noise acceleration
// Obs:     one range per anchor update (sequential, async-friendly)
// Noise:   range sigma scaled by rx_power so weak-signal ranges are down-weighted
#pragma once

#include <Eigen/Dense>

#include <cmath>
#include <string>
#include <vector>

namespace uwb_positioning {

struct Anchor {
  std::string id;
  double x = 0.0, y = 0.0, z = 0.0;
};

// ---------------------------------------------------------------------------
// EKF
// ---------------------------------------------------------------------------
class UwbEkf {
 public:
  static constexpr int Ns = 6;
  using VecN  = Eigen::Matrix<double, Ns, 1>;
  using MatNN = Eigen::Matrix<double, Ns, Ns>;
  using RowN  = Eigen::Matrix<double, 1,  Ns>;

  UwbEkf() : initialized_(false), last_t_(-1.0) {
    x_.setZero();
    P_ = MatNN::Identity() * 100.0;
  }

  // Call once before using the filter.
  //   sigma_a     : acceleration noise spectral density (m/s^2), tunes prediction spread
  //   sigma_r     : range std-dev (m) at the reference rx_power
  //   rx_ref_dbm  : rx_power (dBm) at which sigma_r applies (strong-signal baseline)
  //   rx_decay_db : signal drop (dB) that multiplies range noise by 10x
  //                 (20 dB → noise 10x worse per 20 dB of path loss, 0 dB = disable scaling)
  void configure(double sigma_a, double sigma_r,
                 double rx_ref_dbm, double rx_decay_db) {
    sigma_a_     = sigma_a;
    sigma_r_     = sigma_r;
    rx_ref_dbm_  = rx_ref_dbm;
    rx_decay_db_ = (rx_decay_db > 0.0) ? rx_decay_db : 1e9;
  }

  // Initialise state from a trilaterated position estimate.
  void init(double x, double y, double z, double t) {
    x_ << x, y, z, 0.0, 0.0, 0.0;
    P_ = MatNN::Zero();
    P_.block<3,3>(0,0) = Eigen::Matrix3d::Identity() * 1.0;   // 1 m position uncertainty
    P_.block<3,3>(3,3) = Eigen::Matrix3d::Identity() * 0.01;  // 0.1 m/s velocity uncertainty
    last_t_      = t;
    initialized_ = true;
  }

  bool isInitialized() const { return initialized_; }

  // Constant-velocity prediction to absolute time t (seconds).
  void predict(double t) {
    if (last_t_ < 0.0) { last_t_ = t; return; }
    const double dt = t - last_t_;
    if (dt <= 0.0) return;
    last_t_ = t;

    // State transition matrix F
    MatNN F = MatNN::Identity();
    F(0,3) = dt; F(1,4) = dt; F(2,5) = dt;

    // Discrete white-noise-acceleration process noise Q
    const double q   = sigma_a_ * sigma_a_;
    const double dt2 = dt * dt, dt3 = dt2 * dt, dt4 = dt3 * dt;
    MatNN Q = MatNN::Zero();
    for (int i = 0; i < 3; ++i) {
      Q(i,   i  ) = dt4 / 4.0 * q;
      Q(i,   i+3) = dt3 / 2.0 * q;
      Q(i+3, i  ) = dt3 / 2.0 * q;
      Q(i+3, i+3) = dt2       * q;
    }

    x_ = F * x_;
    P_ = F * P_ * F.transpose() + Q;
  }

  // Sequential EKF measurement update: one range observation from anchor a.
  // Measurement noise sigma is inflated when rx_power_dbm falls below rx_ref_dbm.
  void update(const Anchor& a, double range, double rx_power_dbm) {
    const double dx = x_(0) - a.x;
    const double dy = x_(1) - a.y;
    const double dz = x_(2) - a.z;
    const double r_hat = std::max(std::sqrt(dx*dx + dy*dy + dz*dz), 1e-6);

    // Observation Jacobian H: d(h)/d(state)  [1 x Ns]
    RowN H = RowN::Zero();
    H(0) = dx / r_hat;
    H(1) = dy / r_hat;
    H(2) = dz / r_hat;

    // Range noise: sigma = sigma_r * 10^(max(0, rx_ref - rx) / rx_decay_db)
    // Stronger signal than reference → use sigma_r (no benefit beyond reference).
    const double loss_db    = std::max(0.0, rx_ref_dbm_ - rx_power_dbm);
    const double noise_gain = std::pow(10.0, loss_db / rx_decay_db_);
    const double R          = std::pow(sigma_r_ * noise_gain, 2.0);

    // Innovation
    const double innov = range - r_hat;

    // Kalman gain K  [Ns x 1]
    const double S = (H * P_ * H.transpose())(0, 0) + R;
    const VecN   K = P_ * H.transpose() / S;

    // Joseph form update (numerically stable for P)
    const MatNN I_KH = MatNN::Identity() - K * H;
    x_ += K * innov;
    P_  = I_KH * P_ * I_KH.transpose() + K * R * K.transpose();
  }

  double posX() const { return x_(0); }
  double posY() const { return x_(1); }
  double posZ() const { return x_(2); }

  // 3x3 position sub-block of the covariance matrix.
  Eigen::Matrix3d positionCovariance() const { return P_.block<3,3>(0,0); }

 private:
  VecN  x_;
  MatNN P_;
  bool  initialized_;
  double last_t_;
  double sigma_a_     = 0.3;
  double sigma_r_     = 0.10;
  double rx_ref_dbm_  = -70.0;
  double rx_decay_db_ = 20.0;
};

// ---------------------------------------------------------------------------
// Weighted linear least-squares trilateration
// ---------------------------------------------------------------------------
// Linearises the range equations by subtracting the last anchor's equation
// from all others, then solves the resulting linear system with WLS.
//
//   anchors : known 3-D positions, >= 3 entries required
//   ranges  : measured distances, same order as anchors
//   weights : 1/sigma_i^2 per measurement; pass empty for unit weights
//
// Returns true and writes (x_out, y_out, z_out) on success.
inline bool trilaterate(const std::vector<Anchor>& anchors,
                        const std::vector<double>&  ranges,
                        const std::vector<double>&  weights,
                        double& x_out, double& y_out, double& z_out) {
  const int M = static_cast<int>(anchors.size());
  if (M < 3 || static_cast<int>(ranges.size()) != M) return false;

  // Reference anchor (last entry): subtract its equation from the others.
  const Anchor& aN  = anchors[M-1];
  const double  rN  = ranges[M-1];
  const double  rN2 = rN * rN;
  const double  dN2 = aN.x*aN.x + aN.y*aN.y + aN.z*aN.z;

  Eigen::MatrixXd A(M-1, 3);
  Eigen::VectorXd b(M-1), w(M-1);
  for (int i = 0; i < M-1; ++i) {
    const Anchor& ai  = anchors[i];
    const double  ri  = ranges[i];
    A(i,0) = 2.0 * (aN.x - ai.x);
    A(i,1) = 2.0 * (aN.y - ai.y);
    A(i,2) = 2.0 * (aN.z - ai.z);
    b(i)   = ri*ri - rN2 - (ai.x*ai.x + ai.y*ai.y + ai.z*ai.z) + dN2;
    w(i)   = weights.empty() ? 1.0 : weights[i];
  }

  const Eigen::MatrixXd W    = w.asDiagonal();
  const Eigen::MatrixXd AtWA = A.transpose() * W * A;
  if (std::abs(AtWA.determinant()) < 1e-10) return false;  // degenerate geometry

  const Eigen::Vector3d sol = AtWA.ldlt().solve(A.transpose() * W * b);
  x_out = sol(0); y_out = sol(1); z_out = sol(2);
  return true;
}

}  // namespace uwb_positioning
