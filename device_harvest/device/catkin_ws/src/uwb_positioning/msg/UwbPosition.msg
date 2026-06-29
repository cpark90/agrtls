std_msgs/Header header

float64 x
float64 y
float64 z

# 3x3 position covariance (row-major, metres^2).
# Diagonal entries (0,4,8) = variance in x, y, z from the EKF.
float64[9] position_covariance

# Last measured range to each anchor (m), indexed by anchor order in params.yaml.
# Present even when EKF is uninitialised so raw ranges are always logged.
float64[] anchor_distances
