FROM ros:noetic-ros-base

# C++ toolchain. roscpp, message_filters, and message generation ship with
# ros-base; serial is handled in-tree (POSIX termios), so no serial package needed.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        python3-catkin-tools \
    && rm -rf /var/lib/apt/lists/*

# Workspace is volume-mounted at runtime (see docker-compose.yml); this just
# guarantees the directories exist.
RUN mkdir -p /catkin_ws/src /data
WORKDIR /catkin_ws

# Source ROS (and the workspace overlay if built) for every shell.
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
