# uwb_positioning 설계 문서

UWB32 모듈로부터 앵커-거리 데이터를 시리얼로 수신해 태그의 3D 위치를 추정하는 패키지의 설계를 기술한다.

---

## 1. 시리얼 프레임 포맷

장치는 앵커 하나의 측정값을 한 줄에 ASCII로 출력한다.

```
<device_id>,<range_m>,<rx_power_dbm>,<timestamp>\n
```

예시:
```
A0,1.234,-78.5,1023456
A1,2.187,-71.2,1023458
A2,3.011,-80.0,1023461
A3,2.890,-74.1,1023463
```

| 필드 | 타입 | 설명 |
|------|------|------|
| `device_id` | string | 앵커 식별자. 앵커 설정 파일(`config/anchors/*.yaml`)의 `id`와 매핑 |
| `range_m` | float | 태그-앵커 간 측정 거리 (미터) |
| `rx_power_dbm` | float | 수신 신호 세기 (dBm). EKF 노이즈 모델 및 trilateration 가중치에 사용 |
| `timestamp` | uint/float | 장치 내부 타임스탬프 (단위 미확인). EKF predict의 dt 계산에 사용 |

앵커들은 각자 다른 시점에, 다른 주기로 프레임을 전송한다. 노드는 프레임이 도착하는 즉시 처리한다.

### 타임스탬프 처리

장치 타임스탬프는 EKF `predict()` 단계의 `dt` 계산에 활용된다. ROS 도착 시각(`ros::Time::now()`) 대신 장치 클락을 사용하면 OS 스케줄링 지터나 시리얼 버퍼 지연에 의한 dt 오차가 제거된다.

```
t_device = device_ts_raw × timestamp_scale   (단위: 초)
EKF.predict(t_device)                         ← dt = t_device - 이전 t_device
header.stamp = ros::Time::now()               ← 에포크 보정 전까지 도착 시각 사용
```

`timestamp_scale`은 `params.yaml`의 `ekf/timestamp_scale`로 설정한다 (기본값 `1e-3`, 밀리초 가정).

> **미결**: 장치 타임스탬프의 단위(ms/µs)와 에포크(부팅 기준/UTC 등)를 펌웨어 문서에서 확인해야 한다. 에포크가 확인되면 `header.stamp`도 장치 클락으로 전환하고 오프셋을 보정한다.

---

## 2. 전체 데이터 흐름

```
시리얼 프레임 수신
       │
  parseFrame()                  ← device_id, range, rx_power, timestamp 추출
       │                           t_device = timestamp × timestamp_scale (초)
  anchor_idx 조회               ← 앵커 설정 파일에서 로드된 AnchorMap에서 3D 위치 lookup
       │
  ┌────┴────────────────────────────────────────────────────────┐
  │ EKF 미초기화                                                  │
  │   cache[id] = (range, rx_power)                             │
  │   유효 앵커 >= 3개?                                          │
  │     YES → weighted trilateration → EKF 초기화               │
  │     NO  → 다음 프레임 대기                                   │
  └────┬────────────────────────────────────────────────────────┘
       │ EKF 초기화 완료
  EKF predict(t_device)         ← 장치 클락 기준 dt로 상태 전파 (OS 지터 제거)
       │
  EKF update(anchor, range, rx) ← 이 측정값으로 상태 보정
       │
  publish UwbPosition           ← x, y, z, position_covariance[9], anchor_distances[]
```

---

## 3. Trilateration (초기화)

### 목적

EKF는 초기 상태 추정값이 필요하다. 3개 이상의 앵커 측정값이 누적되면 trilateration으로 초기 위치를 계산한다.

### 알고리즘: Weighted Linear Least Squares

**선형화**: 앵커 i의 범위 방정식에서 마지막 앵커(N)의 방정식을 빼면 이차항이 소거된다.

원래 방정식 (앵커 i):
```
(x - axi)² + (y - ayi)² + (z - azi)² = ri²
```

마지막 앵커 방정식을 빼면:
```
2(axN - axi)·x + 2(ayN - ayi)·y + 2(azN - azi)·z
  = ri² - rN² - (axi² + ayi² + azi²) + (axN² + ayN² + azN²)
```

이를 행렬로 표현하면 `Ax = b` 형태의 선형 시스템이 된다.

**가중치 최소자승 풀이 (WLS)**:

```
W = diag(w₀, w₁, ..., w_{N-2})     wi = 1/σᵢ²

x_hat = (AᵀWA)⁻¹ AᵀWb
```

가중치 wi는 rx_power로부터 산출한다 — 신호가 약할수록 가중치가 낮다.

### 기하 조건

| 앵커 수 | 조건 |
|---------|------|
| 3개 | 2D 추정 가능 (z방향 불확실) |
| 4개 이상 | 3D 추정, 과결정 시스템으로 노이즈에 강인 |
| 동일 평면 배치 | `det(AᵀWA) ≈ 0` → 실패, WARN 출력 |

구현: `include/uwb_positioning/trilateration.h` → `trilaterate()`

---

## 4. EKF (Tightly Coupled)

### 4.1 "Tightly Coupled"의 의미

| 방식 | 설명 | 단점 |
|------|------|------|
| Loosely coupled | trilateration으로 위치를 먼저 계산한 뒤 그 위치를 EKF observation으로 사용 | 앵커가 3개 미만이면 위치 계산 불가 |
| **Tightly coupled** | raw range measurement를 직접 EKF observation으로 사용 | 비선형 → EKF 필요 |

본 구현은 tightly coupled — `h_i(x) = ‖x_tag − x_anchor_i‖`를 observation 모델로 사용한다.

### 4.2 상태 벡터

```
x = [x, y, z, vx, vy, vz]ᵀ   (6×1)
```

위치와 속도를 함께 추정한다. 정적 물체에서도 속도 상태를 포함하면 measurement noise에 의한 위치 출력 떨림이 감소한다.

### 4.3 프로세스 모델 (Prediction)

**등속 운동 모델 (Constant Velocity)**:

```
F = [I₃  dt·I₃]   (6×6)
    [0₃    I₃  ]

x_{k|k-1} = F · x_{k-1|k-1}
P_{k|k-1} = F · P_{k-1|k-1} · Fᵀ + Q
```

`dt`는 이전 predict 호출 이후 실제 경과 시간 — 프레임 도착 간격이 일정하지 않아도 자동 보정된다.

**프로세스 노이즈 Q (이산 백색 잡음 가속도 모델)**:

```
         [dt⁴/4  dt³/2]
Q = σₐ² ·[            ] ⊗ I₃
         [dt³/2  dt²  ]
```

`σₐ` (가속도 노이즈 스펙트럴 밀도, m/s²): 높을수록 빠른 움직임을 추적하나 출력이 불안정해지고, 낮을수록 부드럽지만 급격한 움직임에 뒤처진다.

### 4.4 관측 모델 (Update)

**비선형 관측 함수**:
```
h_i(x) = √((x - ax_i)² + (y - ay_i)² + (z - az_i)²)
```

**선형화 (Jacobian)** — 현재 추정점에서 1차 Taylor 전개:

```
        [∂h/∂x  ∂h/∂y  ∂h/∂z  0  0  0]
H_i  =  [                              ]   (1×6)

      = [(x̂-ax_i)/ĥ,  (ŷ-ay_i)/ĥ,  (ẑ-az_i)/ĥ,  0, 0, 0]
```

이 벡터는 태그에서 앵커 방향의 단위벡터다. 앵커가 +x 방향에 있으면 `H = [1, 0, 0, 0, 0, 0]` → x 방향 오차가 거리 오차에 그대로 반영.

**Innovation & Kalman Gain**:
```
innovation  z̃ = r_measured - ĥ
innovation covariance  S = H·P·Hᵀ + R   (스칼라)
Kalman gain  K = P·Hᵀ / S              (6×1)
```

**상태 업데이트 (Joseph form)**:
```
x̂ ← x̂ + K · z̃
P  ← (I - KH)·P·(I - KH)ᵀ + K·R·Kᵀ
```

Joseph form을 사용하는 이유: 부동소수점 반올림 오차가 누적될 때 `P`가 양정치(positive definite)를 잃는 것을 방지한다.

### 4.5 Sequential Update

앵커들은 비동기로 도착하므로 하나의 프레임이 들어올 때마다 1×1 update를 수행한다. Batch update(모든 앵커를 동시에 처리)와 수학적으로 동치이지만 행렬 역산이 없고 도착 순서를 타지 않는다.

```
t_dev=1023.456  "A0,1.234,-78.5,1023456"  → predict(1023.456) → update(A0)
t_dev=1023.458  "A2,3.011,-71.2,1023458"  → predict(1023.458) → update(A2)
t_dev=1023.461  "A1,2.187,-80.0,1023461"  → predict(1023.461) → update(A1)
t_dev=1023.463  "A0,1.238,-78.3,1023463"  → predict(1023.463) → update(A0)
```
(t_dev = timestamp_raw × timestamp_scale, 단위: 초)

### 4.6 측정 노이즈 모델 (rx_power 기반)

rx_power가 약할수록 거리 측정 오차가 커진다. 이를 반영해 R을 가변으로 설정한다.

```
loss_dB = max(0,  rx_ref_dBm − rx_power_dBm)
                  ─────────────────────────
                  기준보다 얼마나 약한가 (0 미만은 클램프)

σ_r(rx) = σ_r_base × 10^(loss_dB / rx_decay_dB)

R = σ_r(rx)²
```

예시 (`rx_ref = -70 dBm`, `rx_decay = 20 dB`, `σ_r_base = 0.1 m`):

| rx_power | loss | 배율 | σ_r | R |
|----------|------|------|-----|---|
| -60 dBm (강) | 0 dB | ×1 | 0.10 m | 0.010 m² |
| -70 dBm (기준) | 0 dB | ×1 | 0.10 m | 0.010 m² |
| -80 dBm | 10 dB | ×3.16 | 0.316 m | 0.100 m² |
| -90 dBm (약) | 20 dB | ×10 | 1.00 m | 1.000 m² |

R이 크면 → Kalman gain K가 작아짐 → 해당 측정값의 영향이 줄어듦 → 자동 다운웨이팅.

`rx_decay_db`를 매우 큰 값(예: 1000)으로 설정하면 rx_power 기반 스케일링이 비활성화된다.

---

## 5. 모듈 구조

헤더 파일별로 단일 책임을 가진다. 향후 필터 고도화(UKF, Particle Filter 등)는 `position_filter.h`의 인터페이스만 구현하면 된다.

| 헤더 | 역할 |
|------|------|
| `serial_port.h` | POSIX termios 래퍼. 포트 열기, select() 기반 readLine, 닫기 |
| `anchor_map.h` | `AnchorMap` 클래스. `id` 문자열로 앵커 인덱스·위치를 O(1) 조회 |
| `frame_parser.h` | `UwbFrame` struct + `parseFrame()`. 시리얼 한 줄 → 구조체 변환 |
| `position_filter.h` | `PositionFilter` 추상 인터페이스. `init / predict / updateRange / getPosition / positionCovariance` |
| `trilateration.h` | `rxToWeight()` + `trilaterate()`. Eigen 기반 WLS, header-only |
| `uwb_ekf.h` | `EkfConfig` + `UwbEkf`. `PositionFilter`의 기본 구현체 (6-state EKF) |

### 필터 교체 방법

1. `position_filter.h`의 `PositionFilter`를 상속해 새 클래스를 작성한다.
2. `src/uwb_node.cpp`의 `makeFilter()` 팩토리에 `else if (type == "ukf") { ... }` 케이스를 추가한다.
3. `config/params.yaml`의 `filter/type`을 새 타입 이름으로 변경한다.

코드 수정 범위: 신규 헤더 1개 + `uwb_node.cpp`의 `makeFilter()` 내 케이스 1개 추가.

---

## 6. 파라미터 구성

설정은 두 파일로 분리된다. 런타임 파라미터(`params.yaml`)와 배포 환경별 앵커 배치(`config/anchors/*.yaml`)를 독립적으로 관리한다.

### 6.1 런타임 파라미터 (`config/params.yaml`)

```yaml
uwb_node:
  port:         "/dev/serial/by-id/..."  # 실제 by-id 경로로 변경 필수
  baud:         115200
  frame_id:     "uwb"
  topic:        "/uwb/position"
  read_timeout: 1.0                      # 초; ROS 종료 응답성 유지

  filter:
    type: "ekf"                          # makeFilter() 팩토리에 전달되는 식별자

  ekf:
    sigma_a:          0.3     # 가속도 노이즈 (m/s²) — 클수록 빠른 움직임 추적
    sigma_r:          0.10    # 기준 거리 측정 오차 (m, 1σ at rx_ref)
    rx_ref_dbm:      -70.0    # 이 rx_power에서 sigma_r 적용
    rx_decay_db:      20.0    # 이 만큼 신호가 약해지면 노이즈 10배
    timestamp_scale:  1.0e-3  # 장치 타임스탬프 단위 변환 (기본: ms → s)
```

### 6.2 앵커 설정 (`config/anchors/*.yaml`)

앵커 배치는 배포 환경마다 다르다. 별도 YAML 파일로 분리해 launch 인자로 선택한다.

```yaml
# config/anchors/default.yaml
# id: 시리얼 프레임의 device_id와 정확히 일치해야 함
# x, y, z: 월드 프레임 좌표 (미터, 오른손 좌표계, z 위)
anchors:
  - {id: "A0", x: 0.0, y: 0.0, z: 0.0}
  - {id: "A1", x: 3.0, y: 0.0, z: 0.0}
  - {id: "A2", x: 0.0, y: 3.0, z: 0.0}
  - {id: "A3", x: 3.0, y: 3.0, z: 0.0}
```

새 환경 추가: `config/anchors/` 아래에 파일을 추가하기만 하면 된다. 노드 코드나 `params.yaml`은 수정하지 않는다.

### 6.3 launch에서 앵커 파일 선택

```bash
# 기본 (default.yaml)
roslaunch uwb_positioning uwb_positioning.launch

# 다른 환경 선택
roslaunch uwb_positioning uwb_positioning.launch \
  anchors:=$(rospack find uwb_positioning)/config/anchors/lab.yaml
```

launch 파일이 `anchors` 파일을 `ns="uwb_node"`로 로드하므로 노드는 기존과 동일하게 `~anchors` private param으로 읽는다.

### 6.4 튜닝 가이드

| 증상 | 조정 |
|------|------|
| 출력이 너무 떨린다 | `sigma_a` ↓ 또는 `sigma_r` ↑ |
| 움직임에 뒤처진다 | `sigma_a` ↑ |
| 약한 신호의 앵커가 출력을 망친다 | `rx_decay_db` ↓ (더 공격적으로 다운웨이팅) |
| rx_power를 무시하고 싶다 | `rx_decay_db: 1000` |
| EKF predict dt가 이상하다 | `timestamp_scale` 단위 확인 (ms=1e-3, µs=1e-6) |

---

## 7. 출력 메시지 (`msg/UwbPosition.msg`)

```
std_msgs/Header header

float64 x
float64 y
float64 z

float64[9] position_covariance   # 3×3, row-major (m²). [0],[4],[8] = σ²_x, σ²_y, σ²_z

float64[] anchor_distances        # 최근 앵커별 측정 거리 (m), 앵커 설정 파일 순서 기준
```

`position_covariance`는 EKF 공분산 행렬 P의 위치 블록(3×3)이다. 대각 원소가 클수록 추정 불확실성이 높다.

---

## 8. 파일 구조

```
uwb_positioning/
  include/uwb_positioning/
    serial_port.h         # POSIX termios 래퍼 (header-only)
    anchor_map.h          # AnchorMap 클래스 (앵커 ID → 인덱스·위치 O(1) 조회)
    frame_parser.h        # UwbFrame struct + parseFrame() (header-only)
    position_filter.h     # PositionFilter 추상 인터페이스 (필터 교체 스왑 포인트)
    trilateration.h       # rxToWeight() + trilaterate() (header-only, Eigen)
    uwb_ekf.h             # EkfConfig + UwbEkf (PositionFilter 구현, 6-state EKF)
  src/
    uwb_node.cpp          # ROS 노드; 시리얼 루프, loadAnchors, makeFilter, publish
  msg/
    UwbPosition.msg
  config/
    params.yaml           # 런타임 파라미터 (포트, baud, EKF 노이즈)
    anchors/
      default.yaml        # 기본 앵커 배치 (배포 전 실측값으로 교체)
  launch/
    uwb_positioning.launch  # anchors 인자로 배포 환경별 앵커 파일 선택
  DESIGN.md               # 이 문서
```

---

## 9. 미결 사항

| # | 항목 | 조치 |
|---|------|------|
| 1 | 앵커 실측 위치 입력 | `config/anchors/default.yaml` 또는 신규 파일에 실측 좌표 입력. trilateration 정확도 결정 |
| 2 | 타임스탬프 단위·에포크 확인 | `timestamp_scale` 설정; 에포크 확인 후 `header.stamp`도 장치 클락으로 전환 |
| 3 | 2D / 3D 선택 | 앵커가 같은 높이면 z 추정 불안정 — z 고정 옵션 고려 |
