# scale_reader 설계 문서

시리얼 저울로부터 무게 데이터를 수신해 `WeightReading` 메시지를 퍼블리싱하는 패키지의 설계를 기술한다.

---

## 1. 역할 및 책임

```
/dev/serial/by-id/...SCALE
        │
  serial_port.h              ← POSIX termios, select() 기반 timeout
  (readLine)
        │
  parseFrame()               ← ASCII 한 줄 → float (STUB, 교체 필요)
        │
  WeightReading 퍼블리싱     → /scale/weight
```

이 패키지는 시리얼 I/O와 프레임 파싱만 담당한다. 시간 동기화, 기록, 위치와의 결합은 `harvest_recorder`가 한다.

---

## 2. 시리얼 프레임 포맷 (STUB)

> **미확인 상태.** 아래 포맷은 임시 가정이다. 실제 저울 프로토콜 확인 후 `parseFrame()`을 교체해야 한다.

현재 구현 가정: 줄바꿈으로 구분된 ASCII float 한 개.

```
72.450\n
```

### 교체 시 고려 사항

| 항목 | 확인 필요 내용 |
|------|--------------|
| 포맷 | ASCII 라인 vs 바이너리 프레임 |
| 단위 | 저울이 출력하는 단위 (kg, g, lb 등) — `params.yaml`의 `unit`과 일치 여부 |
| 구분자 | 쉼표 구분 vs 공백 vs 고정 길이 필드 |
| 체크섬 | CRC나 LRC 포함 여부 |
| 음수 | 영점 이하 측정값 허용 여부 |
| Stable 출력 | 일부 저울은 측정값이 안정될 때만 출력하고, 불안정 시 별도 상태 코드를 포함 |

교체 방법: `src/scale_reader_node.cpp`의 `parseFrame()` 내부만 수정한다. 함수 시그니처(`const std::string& raw, double& weight`)는 유지한다.

---

## 3. 데이터 흐름

```
시리얼 포트 오픈
       │
  readLine(timeout)          ← select() 기반, timeout 동안 데이터 없으면 false 반환
       │  false → ros::ok() 확인 후 재시도
       │  true
  parseFrame()
       │  실패 → WARN_THROTTLE 출력 후 버림
       │  성공
  WeightReading 생성
  header.stamp = ros::Time::now()
       │
  pub_.publish()             → /scale/weight
```

**타임스탬프**: 현재 프레임 수신 시점에 `ros::Time::now()`를 찍는다. 저울이 자체 타임스탬프를 제공하면 그것을 우선하고 오프셋을 문서화한다 (미결 사항 참조).

**종료 처리**: `ros::ok()`가 false가 되면 루프를 빠져나와 `sp.close()`로 포트를 정상 종료한다. `readLine()`의 timeout이 이 응답성을 보장한다 — timeout 없이 블로킹 read를 사용하면 종료 신호를 받지 못한다.

---

## 4. 출력 메시지 (`msg/WeightReading.msg`)

```
std_msgs/Header header

float64 weight   # 측정 무게
string  unit     # 물리 단위, e.g. "kg" — params.yaml에서 설정
```

`header.stamp`는 `harvest_recorder`의 ApproximateTime 동기화에 사용된다. 이 타임스탬프가 부정확하면 UWB 위치와의 매칭 정확도가 떨어진다.

---

## 5. 시리얼 포트 (`include/scale_reader/serial_port.h`)

POSIX termios 기반 header-only 래퍼. 외부 패키지 의존성 없음.

| 메서드 | 설명 |
|--------|------|
| `open(port, baud)` | 포트 열기, cfmakeraw 설정, 하드웨어 흐름 제어 비활성화 |
| `readLine(line, timeout_s)` | `select()`로 데이터 대기, 줄바꿈까지 읽어 CR 제거 후 반환 |
| `close()` | 파일 디스크립터 정상 닫기 |

지원 baud rate: 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600.

> 바이너리 프레임 프로토콜로 교체할 경우 `readLine()` 대신 고정 길이 또는 구분자 기반 `readFrame()` 메서드를 `serial_port.h`에 추가한다.

---

## 6. 파라미터 (`config/params.yaml`)

```yaml
scale_reader_node:
  port: "/dev/serial/by-id/CHANGE_ME_SCALE"  # 실제 by-id 경로로 변경 필수
  baud: 9600
  frame_id: "scale"
  topic: "/scale/weight"
  unit: "kg"                                  # 저울이 출력하는 물리 단위
  read_timeout: 1.0                           # 초; ROS 종료 응답성 유지
```

`unit`은 코드에서 파싱하지 않는다 — `params.yaml`에서 읽어 메시지에 그대로 실어 보낸다. 저울 출력 단위와 반드시 일치해야 한다.

---

## 7. 파일 구조

```
scale_reader/
  include/scale_reader/
    serial_port.h          # POSIX termios 래퍼 (header-only)
  src/
    scale_reader_node.cpp  # ROS 노드 진입점; 시리얼 루프, parseFrame, publish
  msg/
    WeightReading.msg
  config/
    params.yaml
  launch/
    scale_reader.launch
  DESIGN.md                # 이 문서
```

---

## 8. 미결 사항

| # | 항목 | 조치 |
|---|------|------|
| 1 | **저울 프레임 포맷 미확인** | 실제 프로토콜 확인 후 `parseFrame()` 교체 |
| 2 | **타임스탬프 출처** | 저울이 자체 타임스탬프를 제공하는지 확인. 제공하면 `header.stamp`에 사용하고 오프셋 문서화 |
| 3 | **단위 자동 감지** | 일부 저울은 프레임에 단위를 포함함. 그 경우 `params.yaml`의 `unit` 대신 파싱 값을 사용 |
| 4 | **안정 상태 플래그** | 저울이 측정 안정 여부를 함께 출력하면 `WeightReading.msg`에 `bool stable` 필드 추가 고려 |
