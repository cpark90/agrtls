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

# 중요 지침: 도구(Tools) 및 검색 권한
- 너는 이제 기본 4개 툴(`read`, `write`, `edit`, `bash`) 외에 외부 웹 검색을 할 수 있는 **`mcp` 툴(Parallel Search)**을 완벽하게 사용할 수 있다.
- 인터넷 검색이나 최신 정보가 필요할 때는 절대로 "할 수 없다"고 포기하지 말고, 주저 없이 `mcp` 관련 툴(`web_search` 또는 `parallel-search`)을 호출하여 결과를 받아와라.
- 추론과정은 생략하고 결과위주로 전달해라.
