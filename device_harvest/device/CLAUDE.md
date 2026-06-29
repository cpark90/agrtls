# CLAUDE.md

이 파일은 Claude Code가 저장소를 파악하기 위한 안내 문서다. 세션 시작 시 반드시 읽고, 결정 사항이 바뀌면 즉시 업데이트한다.

## 무엇을 만드는가

UWB32 측위 모듈과 저울 두 개의 시리얼 장치를 읽어 두 스트림을 시간 동기화한 뒤 기록·재퍼블리싱하는 ROS Noetic 시스템이다. 모든 실행은 호스트의 `ros:noetic-ros-base` 이미지를 기반으로 한 컨테이너 안에서 이루어진다. **최종 산출물은 Docker 이미지 안에서 동작하는 시스템**이다 — Bazel 빌드나 호스트 직접 설치가 아니다.

## 아키텍처

```
  /dev/serial/by-id/...UWB   ──▶ uwb_node          ──▶ /uwb/position   ┐
                                  (uwb_positioning)                      ├─▶ recorder ─▶ CSV (/data)
  /dev/serial/by-id/...SCALE ──▶ scale_reader_node  ──▶ /scale/weight   ┘            └─▶ /synced/reading
                                  (scale_reader)          (harvest_recorder)
```

각 패키지는 단일 책임을 가진다:

- **`uwb_positioning`** — UWB32 모듈로부터 앵커-거리 프레임을 시리얼로 수신, 삼변측량(trilateration)으로 초기 위치를 추정한 뒤 tightly-coupled EKF로 위치를 추정해 `UwbPosition`을 `/uwb/position`에 퍼블리싱한다. 설계 상세는 `catkin_ws/src/uwb_positioning/DESIGN.md` 참조.
- **`scale_reader`** — 시리얼 저울로부터 무게 프레임을 수신해 `WeightReading`을 `/scale/weight`에 퍼블리싱한다.
- **`harvest_recorder`** — 두 토픽을 구독해 `message_filters` ApproximateTime으로 시간 동기화한 뒤 CSV로 기록하고 결합된 `HarvestReading`을 `/synced/reading`에 재퍼블리싱한다.

세 노드는 `harvest_recorder/launch/bringup.launch`로 함께 시작된다.

## 확정된 결정 사항 (재논의 전 반드시 질문할 것)

- **빌드 시스템: catkin** (Bazel은 검토 후 제외).
- **언어: C++ / roscpp.**
- **시리얼 I/O는 in-tree** POSIX termios 구현 (`include/<pkg>/serial_port.h`). 외부 시리얼 패키지 없음 — Docker 빌드 의존성을 최소화하기 위함. 변경 시 package.xml 및 Dockerfile도 같이 수정.
- **`harvest_recorder`는 ROS 토픽을 소비** (자체 시리얼 포트 없음).
- **개발·실행 모두 Docker** — `docker compose`로 workspace를 볼륨 마운트해서 편집이 즉시 반영된다.

## 저장소 레이아웃

```
docker/
  Dockerfile              # FROM ros:noetic-ros-base + C++ 툴체인
  entrypoint.sh           # ROS + workspace overlay 소싱
  docker-compose.yml      # catkin_ws 마운트, 시리얼 장치 패스스루, host 네트워킹
catkin_ws/src/
  uwb_positioning/
    package.xml / CMakeLists.txt
    msg/UwbPosition.msg             # Header + x,y,z + position_covariance[9] + anchor_distances[]
    include/uwb_positioning/serial_port.h   # header-only termios 래퍼
    include/uwb_positioning/uwb_ekf.h       # EKF + trilateration (header-only, Eigen)
    src/uwb_node.cpp                # 시리얼 읽기 → parseFrame → EKF predict/update → publish
    config/params.yaml              # 포트, baud, 앵커 위치, EKF 노이즈 파라미터
    launch/uwb_positioning.launch
    DESIGN.md                       # 설계 상세 문서
  scale_reader/
    package.xml / CMakeLists.txt
    msg/WeightReading.msg           # Header + weight + unit
    include/scale_reader/serial_port.h
    src/scale_reader_node.cpp       # 시리얼 읽기 → parseFrame (STUB) → publish
    config/params.yaml
    launch/scale_reader.launch
    DESIGN.md                       # 설계 상세 문서
  harvest_recorder/
    package.xml / CMakeLists.txt
    msg/HarvestReading.msg          # Header + UwbPosition + WeightReading
    src/recorder_node.cpp           # ApproximateTime 동기화 → CSV + publish
    src/sim_harvest.cpp             # 하드웨어 없이 테스트용 fake publisher
    config/params.yaml
    launch/bringup.launch           # 세 노드 전체 실행
    launch/recorder.launch          # recorder만 실행
    DESIGN.md                       # 설계 상세 문서
data/                               # CSV 출력 (git 제외)
```

## 명령어

컨테이너 내부에서 빌드:
```bash
cd /catkin_ws && catkin_make && source devel/setup.bash
```

전체 시스템 실행:
```bash
docker compose -f docker/docker-compose.yml up --build
# 또는 컨테이너 내부에서:
roslaunch harvest_recorder bringup.launch
```

하드웨어 없이 recorder 테스트 (컨테이너 내부):
```bash
roscore &
rosrun harvest_recorder recorder_node &
rosrun harvest_recorder sim_harvest     # 가짜 UwbPosition + WeightReading 퍼블리싱
ls -l /data                             # harvest_*.csv 생성 확인
```

런타임 모니터링:
```bash
rostopic echo /uwb/position
rostopic echo /scale/weight
rostopic hz /synced/reading
```

## 컨벤션

- **모든 센서 메시지는 `header.stamp`를 포함한다.** 시간 동기화가 이에 의존한다. 읽는 시점에 `ros::Time::now()`로 스탬프를 찍는다. 디바이스가 자체 타임스탬프를 제공하는 경우에는 디바이스 클락을 우선하고 오프셋을 문서화한다.
- **디바이스 경로, baud rate, 토픽, slop, CSV 경로를 코드에 하드코딩하지 않는다.** 모두 각 패키지의 `config/params.yaml`에 있으며 private NodeHandle(`pnh.param(...)`)로 읽는다. launch 파일이 yaml을 전역으로 로드하고, 최상위 키가 노드 이름과 일치하므로 private param이 자동으로 해당 노드 섹션에 매핑된다.
- **`/dev/serial/by-id/...`를 사용한다, `/dev/ttyUSB0`가 아니다.** USB 시리얼 디바이스가 두 개일 때 ttyUSB 번호는 재부팅·재연결 시 변경된다.
- **시리얼 읽기는 select() 기반 timeout을 사용**하고 루프에서 `ros::ok()`를 확인한다. ROS 종료 시 블록되지 않으며 포트는 정상 종료된다.
- `slop`은 `Synchronizer::setMaxIntervalDuration`에 매핑된다 (매칭 쌍의 최대 시간 차). 데이터 특성에 따라 달라지므로 params에 노출한다.
- **메시지 필드를 추가·변경할 때** `CMakeLists.txt`, 퍼블리셔, recorder, CSV 헤더를 함께 수정한다 — 항상 동기화 상태를 유지한다.

## Docker / 하드웨어 참고

- `network_mode: host`로 단일 머신에서 ROS_MASTER_URI를 단순하게 유지한다.
- 시리얼 디바이스는 compose의 `devices:`로 패스스루한다. 컨테이너 유저는 `dialout` 그룹에 추가된다. 실제 `by-id` 경로를 채운 뒤 실행한다.
- CSV 출력은 마운트된 `/data` 볼륨에 저장되어 컨테이너 재시작 후에도 유지된다.

## 미결 사항 / TODO (먼저 해결할 것)

1. **저울 프레임 포맷 미확인.** `scale_reader/src/scale_reader_node.cpp`의 `parseFrame()`은 한 줄에 ASCII float 하나를 가정하는 STUB. 실제 프로토콜 확인 후 교체.
2. **타임스탬프 출처** — 두 디바이스 모두 도착 시각 vs 디바이스 제공 타임스탬프. 달성 가능한 동기화 정확도에 영향.
3. **CSV 스키마** — 다운스트림 소비자가 기대하는 정확한 컬럼/단위 확인. 현재 컬럼: `t_uwb, t_scale, dt, x, y, z, anchor_distances, weight, unit`.

## Claude Code 가드레일

- 시리얼 I/O는 명시적 요청 없이는 termios 의존성 없는 방식을 유지한다.
- 코드 수정보다 `params.yaml` 편집을 우선한다.
- C++14, 컴파일러 경고 없이 유지; 노드는 작고 단일 목적으로.
- 메시지 필드 추가 시 해당 필드를 사용하는 모든 패키지를 함께 수정한다.
- **패키지에 새로운 외부 의존성이 추가되면 반드시 Docker 관련 파일을 함께 업데이트한다.**
  - `docker/Dockerfile` — `apt-get install`에 해당 시스템 패키지 추가
  - `docker/docker-compose.yml` — 필요 시 서비스 설정 변경
  - `package.xml` — `<build_depend>`, `<exec_depend>` 선언
  - `CMakeLists.txt` — `find_package()`, `include_directories()` 반영
  - 의존성 추가 체크리스트: ROS 패키지(`ros-noetic-*`) vs 시스템 패키지(`lib*-dev`) 구분 후 Dockerfile에 적절한 형태로 추가.
- .md 파일 작성에만 한글을 사용하고 주석을 포함한 이외에는 영어로 작성.
