# Project agrtls

This project is focused on UWB (Ultra-Wideband) based localization and SLAM (Simultaneous Localization and Mapping) for ground robots. It integrates ESP32-based UWB modules for ranging with advanced SLAM frameworks like ORB-SLAM3 and VINS-Fusion.

## Project Structure

- **esp32/**: Contains the project-specific firmware for UWB Anchors and Tags.
  - `anchor/`: Firmware for UWB anchor nodes.
  - `tag/`: Firmware for UWB tag nodes.
  - `libraries/`: Required Arduino libraries (DW1000, SSD1306).
- **Makerfabs-ESP32-UWB/**: Reference library and examples provided by Makerfabs.
- **orb3/**: ORB-SLAM3 integration using Docker (ROS2 Humble/Jazzy).
- **vinsfusion/**: VINS-Fusion integration using Docker (ROS Noetic).
- **kalibr/**: Calibration configuration and scripts for camera-IMU-UWB systems.

## Development Workflow

### UWB Firmware (ESP32)
1. Use Arduino IDE to open `.ino` files in `esp32/`.
2. Ensure the libraries in `esp32/libraries/` are correctly installed in your Arduino environment.
3. Configure the ESP32 board settings as "ESP32 Dev Module".
4. Flash `anchor.ino` to anchor nodes and `uwb_tag.ino` to tag nodes.

### SLAM & Localization (Docker)
Each SLAM directory (`orb3`, `vinsfusion`) contains `build.sh` and `run.sh` scripts.
- To build the environment: `./build.sh`
- To run the container: `./run.sh`

## Engineering Standards

- **Language**: C++ for firmware and SLAM; Python for auxiliary scripts and data visualization.
- **Style**: Adhere to standard C++ and Python conventions already present in the codebase.
- **Docker**: Always use provided Dockerfiles to ensure environment consistency.
