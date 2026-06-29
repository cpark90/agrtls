#!/usr/bin/env bash
set -e

source "/opt/ros/$ROS_DISTRO/setup.bash"

# Auto-build the workspace on first run (when devel/ does not yet exist).
# Set AUTO_BUILD=0 to skip (e.g. when iterating on params only).
if [ "${AUTO_BUILD:-1}" = "1" ] && [ ! -f /catkin_ws/devel/setup.bash ]; then
    echo "[entrypoint] workspace not built — running catkin_make..."
    cd /catkin_ws && catkin_make
    echo "[entrypoint] build complete."
fi

if [ -f /catkin_ws/devel/setup.bash ]; then
    source /catkin_ws/devel/setup.bash
fi

exec "$@"
