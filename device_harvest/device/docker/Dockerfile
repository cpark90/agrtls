FROM ros:noetic-ros-base

# System dependencies for all three catkin packages.
#
#   libeigen3-dev          uwb_positioning: Eigen3 headers (EKF, trilateration)
#   ros-noetic-cmake-modules  uwb_positioning: provides FindEigen3.cmake for catkin
#   ros-noetic-message-filters  harvest_recorder: ApproximateTime synchronizer
#
# Serial I/O is handled in-tree via POSIX termios — no external serial package needed.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        python3-catkin-tools \
        libeigen3-dev \
        ros-noetic-cmake-modules \
        ros-noetic-message-filters \
    && rm -rf /var/lib/apt/lists/*

# Workspace is volume-mounted at runtime; these directories must exist in the image.
RUN mkdir -p /catkin_ws/src /data
WORKDIR /catkin_ws

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
