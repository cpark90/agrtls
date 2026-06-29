# uwb_positioning 설계 문서

UWB32 모듈로부터 앵커-거리 데이터를 시리얼로 수신해 태그의 3D 위치를 추정하는 패키지의 설계를 기술한다.

---

## 1. 시리얼 프레임 포맷

장치는 앵커 하나의 측정값을 한 줄에 ASCII로 출력한다.

```
<device_id>,<range_m>,<rx_power_dbm>\n
```

예시:
```
A0,1.234,-78.5
A1,2.187,-71.2
A2,3.011,-80.0
A3,2.890,-74.1
```

| 필드 | 타입 | 설명 |
|------|------|------|
| `device_id` | string | 앵커 식별자. `params.yaml`의 앵커 테이블과 매핑 |
| `range_m` | float | 태그-앵커 간 측정 거리 (미터) |
| `rx_power_dbm` | float | 수신 신호 세기 (dBm). 노이즈 모델에 사용 |

앵커들은 각자 다른 시점에, 다른 주기로 프레임을 전송한다. 노드는 프레임이 도착하는 즉시 처리한다.

---

## 2. 전체 데이터 흐름

```
시리얼 프레임 수신
       │
  parseFrame()                  ← device_id, range, rx_power 추출
       │
  anchor_idx 조회               ← params.yaml의 앵커 테이블에서 3D 위치 lookup
       │
  ┌────┴────────────────────────────────────────────────────────┐
  │ EKF 미초기화                                                  │
  │   cache[id] = (range, rx_power)                             │
  │   유효 앵커 >= 3개?                                          │
  │     YES → weighted trilateration → EKF 초기화               │
  │     NO  → 다음 프레임 대기                                   │
  └────┬────────────────────────────────────────────────────────┘
       │ EKF 초기화 완료
  EKF predict(t_now)            ← 마지막 predict 이후 경과 시간만큼 상태 전파
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

구현: `include/uwb_positioning/uwb_ekf.h` → `trilaterate()`

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
t=0.00  "A0,1.234,-78.5"  → predict(0.00) → update(A0)
t=0.01  "A2,3.011,-71.2"  → predict(0.01) → update(A2)
t=0.01  "A1,2.187,-80.0"  → predict(0.01) → update(A1)
t=0.02  "A0,1.238,-78.3"  → predict(0.02) → update(A0)  ← A0만 다시 도착해도 OK
```

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

## 5. 파라미터 (`config/params.yaml`)

```yaml
uwb_node:
  port:         "/dev/serial/by-id/..."  # 실제 by-id 경로로 변경 필수
  baud:         115200
  frame_id:     "uwb"
  topic:        "/uwb/position"
  read_timeout: 1.0                      # 초; ROS 종료 응답성 유지

  anchors:                               # device_id는 시리얼 프레임의 첫 필드와 일치해야 함
    - {id: "A0", x: 0.0, y: 0.0, z: 0.0}
    - {id: "A1", x: 3.0, y: 0.0, z: 0.0}
    - {id: "A2", x: 0.0, y: 3.0, z: 0.0}
    - {id: "A3", x: 3.0, y: 3.0, z: 0.0}

  ekf:
    sigma_a:     0.3     # 가속도 노이즈 (m/s²) — 클수록 빠른 움직임 추적
    sigma_r:     0.10    # 기준 거리 측정 오차 (m, 1σ at rx_ref)
    rx_ref_dbm:  -70.0   # 이 rx_power에서 sigma_r 적용
    rx_decay_db: 20.0    # 이 만큼 신호가 약해지면 노이즈 10배
```

### 튜닝 가이드

| 증상 | 조정 |
|------|------|
| 출력이 너무 떨린다 | `sigma_a` ↓ 또는 `sigma_r` ↑ |
| 움직임에 뒤처진다 | `sigma_a` ↑ |
| 약한 신호의 앵커가 출력을 망친다 | `rx_decay_db` ↓ (더 공격적으로 다운웨이팅) |
| rx_power를 무시하고 싶다 | `rx_decay_db: 1000` |

---

## 6. 출력 메시지 (`msg/UwbPosition.msg`)

```
std_msgs/Header header

float64 x
float64 y
float64 z

float64[9] position_covariance   # 3×3, row-major (m²). [0],[4],[8] = σ²_x, σ²_y, σ²_z

float64[] anchor_distances        # 최근 앵커별 측정 거리 (m), params.yaml 앵커 순서 기준
```

`position_covariance`는 EKF 공분산 행렬 P의 위치 블록(3×3)이다. 대각 원소가 클수록 추정 불확실성이 높다.

---

## 7. 파일 구조

```
uwb_positioning/
  include/uwb_positioning/
    serial_port.h    # POSIX termios 래퍼 (header-only)
    uwb_ekf.h        # UwbEkf 클래스 + trilaterate() 함수 (header-only, Eigen)
  src/
    uwb_node.cpp     # ROS 노드 진입점; 시리얼 루프, 앵커 lookup, EKF 호출
  msg/
    UwbPosition.msg
  config/
    params.yaml
  launch/
    uwb_positioning.launch
  DESIGN.md          # 이 문서
```

---

## 8. 미결 사항

| # | 항목 | 영향 |
|---|------|------|
| 1 | 앵커 실측 위치 입력 | trilateration 정확도 결정 |
| 2 | UWB32 실제 프레임 포맷 확인 | `parseFrame()` 교체 필요 |
| 3 | 타임스탬프 출처 (도착 시각 vs 장치 제공) | 동기화 정확도에 영향 |
| 4 | 2D / 3D 선택 | 앵커가 같은 높이면 z 추정 불안정 — z 고정 옵션 고려 |
