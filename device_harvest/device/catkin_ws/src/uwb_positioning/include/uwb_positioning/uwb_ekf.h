// Extended Kalman Filter — concrete PositionFilter implementation.
//
// State   : [x, y, z, vx, vy, vz]^T  (6-dim)
// Process : constant-velocity model with white-noise acceleration
// Obs     : range to one anchor at a time (sequential, async-friendly)
//           h_i(x) = ||x_tag − x_anchor_i||  (nonlinear → EKF)
// Noise   : range sigma scaled by rx_power (weaker signal → larger R)
// P update: Joseph form for numerical stability
//
// Upgrade path: replace this file with ukf_filter.h / particle_filter.h and
// update the factory in uwb_node.cpp — the node itself does not change.
#pragma once

#include <Eigen/Dense>
#include <array>
#include <cmath>
#include <stdexcept>

#include "position_filter.h"

namespace uwb_positioning {

struct EkfConfig {
  double sigma_a     = 0.3;    // acceleration noise spectral density (m/s²)
  double sigma_r     = 0.10;   // base range std-dev (m) at rx_ref_dbm
  double rx_ref_dbm  = -70.0;  // rx_power at which sigma_r applies
  double rx_decay_db = 20.0;   // noise ×10 per this many dB of signal loss
                                // (set to large value, e.g. 1000, to disable)
};

class UwbEkf : public PositionFilter {
 public:
  static constexpr int Ns = 6;
  using VecN  = Eigen::Matrix<double, Ns, 1>;
  using MatNN = Eigen::Matrix<double, Ns, Ns>;
  using RowN  = Eigen::Matrix<double, 1,  Ns>;

  explicit UwbEkf(const EkfConfig& cfg = EkfConfig())
      : cfg_(cfg), initialized_(false), last_t_(-1.0) {
    x_.setZero();
    P_ = MatNN::Identity() * 100.0;
  }

  void configure(const EkfConfig& cfg) { cfg_ = cfg; }

  // ---- PositionFilter interface ----

  void init(double x, double y, double z, double t_seconds) override {
    x_ << x, y, z, 0.0, 0.0, 0.0;
    P_ = MatNN::Zero();
    P_.block<3, 3>(0, 0) = Eigen::Matrix3d::Identity() * 1.0;   // 1 m pos uncertainty
    P_.block<3, 3>(3, 3) = Eigen::Matrix3d::Identity() * 0.01;  // 0.1 m/s vel uncertainty
    last_t_      = t_seconds;
    initialized_ = true;
  }

  bool isInitialized() const override { return initialized_; }

  // Constant-velocity predict to absolute device time t_seconds.
  void predict(double t_seconds) override {
    if (last_t_ < 0.0) { last_t_ = t_seconds; return; }
    const double dt = t_seconds - last_t_;
    if (dt <= 0.0) return;
    last_t_ = t_seconds;

    // State transition F
    MatNN F = MatNN::Identity();
    F(0, 3) = dt; F(1, 4) = dt; F(2, 5) = dt;

    // Discrete white-noise-acceleration process noise Q
    const double q   = cfg_.sigma_a * cfg_.sigma_a;
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

  // Sequential EKF update with one range observation.
  void updateRange(const Anchor& a, double range_m, double rx_power_dbm) override {
    const double dx    = x_(0) - a.x;
    const double dy    = x_(1) - a.y;
    const double dz    = x_(2) - a.z;
    const double r_hat = std::max(std::sqrt(dx*dx + dy*dy + dz*dz), 1e-6);

    // Observation Jacobian H = d(h)/d(state)  [1 × Ns]
    RowN H = RowN::Zero();
    H(0) = dx / r_hat;
    H(1) = dy / r_hat;
    H(2) = dz / r_hat;

    // Range noise: sigma = sigma_r × 10^(max(0, rx_ref − rx) / rx_decay_db)
    const double loss_db    = std::max(0.0, cfg_.rx_ref_dbm - rx_power_dbm);
    const double noise_gain = std::pow(10.0, loss_db / cfg_.rx_decay_db);
    const double R          = std::pow(cfg_.sigma_r * noise_gain, 2.0);

    // Innovation
    const double innov = range_m - r_hat;

    // Kalman gain  [Ns × 1]
    const double S = (H * P_ * H.transpose())(0, 0) + R;
    const VecN   K = P_ * H.transpose() / S;

    // Joseph-form update (numerically stable, preserves P symmetry and positive-definiteness)
    const MatNN I_KH = MatNN::Identity() - K * H;
    x_ += K * innov;
    P_  = I_KH * P_ * I_KH.transpose() + K * R * K.transpose();
  }

  void getPosition(double& x, double& y, double& z) const override {
    x = x_(0); y = x_(1); z = x_(2);
  }

  // 3×3 position covariance, row-major.
  std::array<double, 9> positionCovariance() const override {
    std::array<double, 9> out;
    const Eigen::Matrix3d cov = P_.block<3, 3>(0, 0);
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        out[static_cast<size_t>(r * 3 + c)] = cov(r, c);
    return out;
  }

 private:
  EkfConfig cfg_;
  VecN      x_;
  MatNN     P_;
  bool      initialized_;
  double    last_t_;
};

}  // namespace uwb_positioning
