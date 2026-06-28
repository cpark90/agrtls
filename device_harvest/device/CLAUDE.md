# CLAUDE.md

This file orients Claude Code in this repository. Read it at the start of every session.
Keep it updated as decisions change.

## What we're building

A ROS Noetic system that reads a UWB32 ranging module and a weight scale over serial,
time-aligns the two streams, and records/republishes the result. Everything runs inside
a container based on the host's `ros:noetic-ros-base` image. **The final deliverable is
the running system inside that Docker image** — not a Bazel build, not an on-host setup.

## Architecture

```
  /dev/serial/by-id/...UWB   ──▶ uwb_node          ──▶ /uwb/position   ┐
                                  (uwb_positioning)                      ├─▶ recorder ─▶ CSV (/data)
  /dev/serial/by-id/...SCALE ──▶ scale_reader_node  ──▶ /scale/weight   ┘            └─▶ /synced/reading
                                  (scale_reader)          (harvest_recorder)
```

Three catkin packages, each with a single purpose:

- **`uwb_positioning`** — reads anchor-distance frames from a UWB32 module over serial,
  computes tag position via multilateration, publishes `UwbPosition` on `/uwb/position`.
- **`scale_reader`** — reads weight frames from a serial scale, publishes `WeightReading`
  on `/scale/weight`.
- **`harvest_recorder`** — subscribes to both topics, time-aligns them with
  `message_filters` ApproximateTime, writes each matched pair to CSV, and republishes
  combined `HarvestReading` on `/synced/reading`.

All three nodes are started together by `harvest_recorder/launch/bringup.launch`.

## Settled decisions (don't relitigate without asking)

- **Build system: catkin** (not Bazel). Bazel was considered and dropped.
- **Language: C++ / roscpp.**
- **Serial I/O is in-tree** via POSIX termios (`include/<pkg>/serial_port.h`). No external
  serial package — keeps the Docker build dependency-free. Swap to the `serial` (wjwwood)
  API only if there's a concrete reason; update package.xml and Dockerfile if you do.
- **`harvest_recorder` consumes ROS topics** from the other two packages (not its own
  serial port).
- **Dev + runtime both happen in Docker** via `docker compose`, with the workspace
  volume-mounted so edits are live and rebuilds are fast.

## Repo layout

```
docker/
  Dockerfile              # FROM ros:noetic-ros-base + C++ toolchain
  entrypoint.sh           # sources ROS + workspace overlay
  docker-compose.yml      # mounts catkin_ws, passes through serial devices, host networking
catkin_ws/src/
  uwb_positioning/
    package.xml / CMakeLists.txt
    msg/UwbPosition.msg             # Header + x,y,z + anchor_distances[]
    include/uwb_positioning/serial_port.h   # header-only termios wrapper
    src/uwb_node.cpp                # serial read → parseFrame (STUB) → multilaterate (STUB)
    config/params.yaml              # port, baud, anchor positions, topic
    launch/uwb_positioning.launch
  scale_reader/
    package.xml / CMakeLists.txt
    msg/WeightReading.msg           # Header + weight + unit
    include/scale_reader/serial_port.h
    src/scale_reader_node.cpp       # serial read → parseFrame (STUB) → publish
    config/params.yaml              # port, baud, unit, topic
    launch/scale_reader.launch
  harvest_recorder/
    package.xml / CMakeLists.txt
    msg/HarvestReading.msg          # Header + UwbPosition + WeightReading
    src/recorder_node.cpp           # ApproximateTime sync → CSV + publish
    src/sim_harvest.cpp             # fake publisher for hardware-free testing
    config/params.yaml              # topics, slop, csv_path, synced_topic
    launch/bringup.launch           # starts all three nodes
    launch/recorder.launch          # recorder only (nodes already running)
data/                               # CSV output (git-ignored)
```

## Commands

Build (inside the container):
```bash
cd /catkin_ws && catkin_make && source devel/setup.bash
```

Run the whole system:
```bash
docker compose -f docker/docker-compose.yml up --build
# or, once inside an interactive container:
roslaunch harvest_recorder bringup.launch
```

Test the recorder without hardware (inside the container):
```bash
roscore &
rosrun harvest_recorder recorder_node &
rosrun harvest_recorder sim_harvest     # publishes fake UwbPosition + WeightReading
ls -l /data                             # harvest_*.csv rows should appear
```

Inspect at runtime:
```bash
rostopic echo /uwb/position
rostopic echo /scale/weight
rostopic hz /synced/reading
```

## Conventions

- **Every sensor message carries `header.stamp`.** Time alignment depends on it. Stamp at
  the moment of read (`ros::Time::now()`), unless the device supplies its own timestamp —
  then prefer the device clock and document the offset.
- **Never hardcode device paths, baud rates, topics, slop, or the CSV path in code.** They
  all live in each package's `config/params.yaml` and are read via the private NodeHandle
  (`pnh.param(...)`). The launch file loads yaml globally; because each top-level key equals
  a node name, a private param resolves to that node's section automatically.
- **Use `/dev/serial/by-id/...`, not `/dev/ttyUSB0`.** With two USB serial devices the
  ttyUSB numbering is not stable across reboots/replug.
- **Serial reads use a select()-based timeout** and the loop checks `ros::ok()`; never
  block ROS shutdown. The port is closed cleanly on exit.
- `slop` maps to `Synchronizer::setMaxIntervalDuration` (max time spread within a matched
  set). It's data-dependent — expose it in params, don't bury it.
- When you add/change a message field, update `CMakeLists.txt` (if needed), the publisher,
  the recorder, and the CSV header together — they must stay in sync.

## Docker / hardware notes

- `network_mode: host` keeps ROS_MASTER_URI trivial for a single machine.
- Serial devices are passed through under `devices:` in compose; the container user is
  added to `dialout` for port permissions. Fill in the real `by-id` paths before running.
- CSV output goes to a mounted `/data` volume so recordings survive container restarts.

## Open questions / TODO (resolve these first)

1. **UWB32 frame format is unknown.** `parseFrame()` in `uwb_positioning/src/uwb_node.cpp`
   is a STUB (one comma-separated float per anchor per line). Confirm the actual protocol,
   then replace the stub. Also implement `multilaterate()` once anchor positions are
   measured and entered in `uwb_positioning/config/params.yaml`.
2. **Scale frame format is unknown.** `parseFrame()` in `scale_reader/src/scale_reader_node.cpp`
   is a STUB (one ASCII float per line). Confirm the actual protocol and replace.
3. **Timestamp source** — arrival-time vs device-supplied for both devices. Affects
   achievable sync accuracy.
4. **CSV schema** — confirm the exact columns/units the downstream consumer expects.
   Current columns: `t_uwb, t_scale, dt, x, y, z, anchor_distances, weight, unit`.

## Guardrails for Claude Code

- Keep serial I/O dependency-free (termios) unless explicitly asked to switch.
- Prefer editing `params.yaml` over touching node code for configuration changes.
- C++14, no compiler warnings if avoidable; keep nodes small and single-purpose.
- When adding a message field, update all three packages that touch it together.
