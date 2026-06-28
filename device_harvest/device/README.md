# serial_sync

ROS Noetic system (C++/roscpp): two serial readers + one time-aligning recorder,
running in a container based on `ros:noetic-ros-base`. Built with catkin, developed
via Docker Compose. Serial I/O is handled in-tree with POSIX termios — no external
serial package.

## Quick start

```bash
# 1. Edit docker/docker-compose.yml: uncomment `devices:` and set real by-id paths.
# 2. Edit catkin_ws/src/serial_sync/config/params.yaml: set ports/baud.
# 3. Bring it up:
docker compose -f docker/docker-compose.yml up --build
```

Inside the container:

```bash
cd /catkin_ws && catkin_make && source devel/setup.bash
roslaunch serial_sync bringup.launch
```

## Test without hardware

```bash
roscore &
rosrun serial_sync recorder_node &
rosrun serial_sync sim_reading
ls -l /data    # CSV appears here
```

See `CLAUDE.md` for architecture, conventions, and the open questions to resolve first
(serial frame format, timestamp source, CSV schema).
