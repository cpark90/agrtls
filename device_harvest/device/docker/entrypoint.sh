#!/usr/bin/env bash
set -e

# Always source the base ROS environment.
source "/opt/ros/$ROS_DISTRO/setup.bash"

# Source the workspace overlay if it has already been built.
if [ -f /catkin_ws/devel/setup.bash ]; then
    source /catkin_ws/devel/setup.bash
fi

exec "$@"
