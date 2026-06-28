std_msgs/Header header

float64 x
float64 y
float64 z

# Distance to each anchor (m), same order as anchors[] in params.yaml.
# Populated even when position estimation fails so raw ranges are always logged.
float64[] anchor_distances
