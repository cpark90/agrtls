# device_harvest / device

UWB32 측위 모듈과 저울을 시리얼로 읽어 두 스트림을 시간 동기화한 뒤 CSV로 기록하고 재퍼블리싱하는 ROS Noetic 시스템이다. C++/roscpp, catkin 빌드, Docker Compose로 실행한다. 시리얼 I/O는 POSIX termios 기반 in-tree 구현 — 외부 패키지 의존성 없음.

## 패키지 구조

| 패키지 | 역할 | 퍼블리시 토픽 |
|--------|------|--------------|
| `uwb_positioning` | UWB32 시리얼 → trilateration + EKF → 위치 추정 | `/uwb/position` |
| `scale_reader` | 저울 시리얼 → 무게 파싱 | `/scale/weight` |
| `harvest_recorder` | 두 토픽 시간 동기화 → CSV 기록 + 재퍼블리싱 | `/synced/reading` |

## 빠른 시작

```bash
# 1. docker/docker-compose.yml: devices: 항목의 by-id 경로를 실제 값으로 변경
# 2. catkin_ws/src/uwb_positioning/config/params.yaml: 앵커 위치 및 포트 설정
# 3. catkin_ws/src/scale_reader/config/params.yaml: 포트 설정
docker compose -f docker/docker-compose.yml up --build
```

컨테이너 내부에서 직접 실행:

```bash
cd /catkin_ws && catkin_make && source devel/setup.bash
roslaunch harvest_recorder bringup.launch
```

## 하드웨어 없이 테스트

```bash
roscore &
rosrun harvest_recorder recorder_node &
rosrun harvest_recorder sim_harvest    # 가짜 UwbPosition + WeightReading 퍼블리싱
ls -l /data                            # harvest_*.csv 생성 확인
```

## 참고 문서

- `CLAUDE.md` — 아키텍처, 컨벤션, 미결 사항
- `catkin_ws/src/uwb_positioning/DESIGN.md` — EKF 및 trilateration 설계 상세
