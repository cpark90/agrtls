// Abstract position filter interface.
//
// The node holds a PositionFilter* and never depends on a concrete type.
// To swap or upgrade the estimator (EKF → UKF → particle filter):
//   1. Create a new class that inherits PositionFilter and implements all pure virtuals.
//   2. Add a case to makeFilter() in uwb_node.cpp.
//   3. Set filter/type in params.yaml.
//   Nothing else in the node changes.
#pragma once

#include <array>
#include "anchor_map.h"

namespace uwb_positioning {

class PositionFilter {
 public:
  virtual ~PositionFilter() = default;

  // Initialise estimator state from a known position.
  // t_seconds: device clock time of this initialisation (seconds).
  virtual void init(double x, double y, double z, double t_seconds) = 0;

  virtual bool isInitialized() const = 0;

  // Propagate state forward to absolute device time t_seconds.
  // The filter computes dt = t_seconds - last_predict_time internally.
  virtual void predict(double t_seconds) = 0;

  // Incorporate one range observation from anchor a.
  // rx_power_dbm drives measurement noise weighting (stronger = tighter bound).
  virtual void updateRange(const Anchor& a,
                           double range_m,
                           double rx_power_dbm) = 0;

  virtual void getPosition(double& x, double& y, double& z) const = 0;

  // 3×3 position covariance, row-major, metres².
  // Diagonal entries [0],[4],[8] = σ²_x, σ²_y, σ²_z.
  virtual std::array<double, 9> positionCovariance() const = 0;
};

}  // namespace uwb_positioning
