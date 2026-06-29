# harvest_recorder 설계 문서

UWB 위치와 저울 무게 두 스트림을 시간 동기화한 뒤 CSV로 기록하고 결합 메시지를 퍼블리싱하는 패키지의 설계를 기술한다.

---

## 1. 역할 및 책임

```
/uwb/position  (UwbPosition)  ─┐
                                ├─▶ ApproximateTime 동기화 ─▶ CSV 기록
/scale/weight  (WeightReading) ─┘                          └─▶ /synced/reading (HarvestReading)
```

이 패키지는 시리얼 I/O나 센서 해석을 하지 않는다. 두 ROS 토픽을 소비해 시간 정렬된 쌍을 만드는 것만 담당한다.

### 패키지 간 의존 관계

```
uwb_positioning ◄──┐
                    ├── harvest_recorder
scale_reader    ◄──┘
```

`harvest_recorder`는 `uwb_positioning`과 `scale_reader`의 메시지 타입에 의존한다. 두 패키지가 먼저 빌드되어야 한다 (catkin이 `package.xml` 의존성으로 자동 처리).

---

## 2. ApproximateTime 동기화

### 왜 ApproximateTime인가

UWB와 저울은 서로 다른 하드웨어에서 독립적으로 동작하므로 타임스탬프가 절대 정확히 일치하지 않는다. `ExactTime`은 동일 타임스탬프를 요구하므로 사실상 매칭이 불가능하다.

`ApproximateTime`은 두 토픽의 타임스탬프 차이가 `slop` 이내인 메시지 쌍을 찾아 콜백을 호출한다.

### 동기화 알고리즘 (message_filters 내부 동작)

```
큐 상태 예시 (slop = 0.1s):

  UWB   : t=1.000  t=1.050  t=1.100  ...
  Scale : t=1.003  t=1.055  t=1.105  ...

  매칭: (UWB t=1.000, Scale t=1.003)  dt=0.003s ✓
         (UWB t=1.050, Scale t=1.055)  dt=0.005s ✓
```

각 채널마다 `queue_size` 크기의 메시지 큐를 유지한다. 새 메시지가 도착하면 상대 채널 큐에서 시간적으로 가장 가까운 메시지를 찾아 쌍을 구성한다.

### slop 설정 기준

```
slop > max(UWB 주기, Scale 주기) / 2
```

예시:
- UWB 10 Hz → 주기 100ms → slop ≥ 50ms
- Scale 5 Hz → 주기 200ms → slop ≥ 100ms
- 권장: 두 값 중 큰 것을 기준으로 여유를 두고 설정

`slop`이 너무 작으면 매칭 실패율이 높아지고, 너무 크면 시간적으로 멀리 떨어진 데이터가 묶인다.

### queue_size 설정 기준

메시지가 몰리거나 일시적 처리 지연이 발생할 때 데이터 손실을 방지한다. 일반적으로 `max_rate_hz × slop × 3` 정도면 충분하다.

---

## 3. 데이터 흐름

```
sub_uwb_.subscribe()   ─┐
                          ├─▶ Synchronizer ─▶ onPair(uwb, scale)
sub_scale_.subscribe() ─┘        │
                                  │  타임스탬프 차이 <= slop?
                                  │    YES
                                  ▼
                          CSV 한 행 기록
                          HarvestReading 메시지 생성
                          pub_.publish()    → /synced/reading
```

`ros::spin()`으로 구동 — 동기화 콜백은 ROS 콜백 큐에서 순차 실행된다. 시리얼 루프가 없으므로 spin 블로킹 방식이 적합하다.

---

## 4. CSV 스키마

파일명: `harvest_YYYYMMDD_HHMMSS.csv` (`csv_path` 디렉토리 안에 생성)

```
t_uwb,t_scale,dt,x,y,z,anchor_distances,weight,unit
```

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `t_uwb` | float64 | UWB 메시지 타임스탬프 (Unix epoch, 초) |
| `t_scale` | float64 | 저울 메시지 타임스탬프 (Unix epoch, 초) |
| `dt` | float64 | `t_uwb - t_scale` (초). 양수면 UWB가 더 나중 |
| `x` | float64 | EKF 추정 x 좌표 (m) |
| `y` | float64 | EKF 추정 y 좌표 (m) |
| `z` | float64 | EKF 추정 z 좌표 (m) |
| `anchor_distances` | 공백 구분 float 목록 | 앵커별 최근 측정 거리 (m) |
| `weight` | float64 | 측정 무게 |
| `unit` | string | 무게 단위 (e.g. "kg") |

> 컬럼 추가·변경 시 이 표, `recorder_node.cpp`의 CSV 헤더 출력, `onPair()` 내 기록 로직을 함께 수정한다.

---

## 5. 출력 메시지

### `msg/HarvestReading.msg`

```
std_msgs/Header header          # stamp = UWB 타임스탬프, frame_id = UWB frame_id

uwb_positioning/UwbPosition uwb
scale_reader/WeightReading  weight
```

`header.stamp`는 UWB 타임스탬프를 사용한다. 저울 타임스탬프는 `weight.header.stamp`에 보존된다.

두 원본 메시지를 그대로 포함하므로 `position_covariance`, `anchor_distances`, `unit` 등 모든 필드에 접근 가능하다.

---

## 6. 하드웨어 없이 테스트 (`src/sim_harvest.cpp`)

`recorder_node` 없이 두 업스트림 노드를 시뮬레이션한다.

```bash
# 컨테이너 내부
roscore &
rosrun harvest_recorder recorder_node &
rosrun harvest_recorder sim_harvest
ls -l /data    # harvest_*.csv 생성 확인
```

`sim_harvest`가 퍼블리싱하는 내용:

| 토픽 | 내용 |
|------|------|
| `/uwb/position` | x가 프레임마다 0.01m씩 증가, y=2.0, z=0.0, 대각 공분산 0.01 m² |
| `/scale/weight` | 10.0~10.9 kg를 순환 (10프레임 주기), unit="kg" |

두 토픽 모두 동일한 `ros::Time::now()`로 스탬프를 찍으므로 `dt ≈ 0`인 매칭 쌍이 생성된다.

---

## 7. 파라미터 (`config/params.yaml`)

```yaml
recorder:
  uwb_topic:    "/uwb/position"
  scale_topic:  "/scale/weight"
  slop:         0.1       # 두 메시지를 한 쌍으로 볼 최대 시간 차 (초)
  queue_size:   50        # 채널당 메시지 큐 크기
  csv_path:     "/data"   # CSV 출력 디렉토리 (없으면 자동 생성)
  publish_synced: true
  synced_topic: "/synced/reading"
```

### slop 튜닝 가이드

| 증상 | 조정 |
|------|------|
| CSV 행이 거의 생성되지 않는다 | `slop` ↑ |
| CSV `dt`가 허용 이상으로 크다 | `slop` ↓ |
| 메시지 큐가 넘쳐 경고가 뜬다 | `queue_size` ↑ |

---

## 8. launch 파일

| 파일 | 용도 |
|------|------|
| `launch/bringup.launch` | `uwb_positioning`, `scale_reader`, `recorder` 세 노드 전체 실행 |
| `launch/recorder.launch` | recorder 노드만 실행. 나머지 두 노드가 이미 실행 중일 때 사용 |

### 앵커 파일 선택

`bringup.launch`는 `anchors` 인자를 받아 `uwb_positioning.launch`로 전달한다. 배포 환경을 바꿀 때 이 인자 하나만 변경하면 된다.

```bash
# 기본 앵커 파일 사용
roslaunch harvest_recorder bringup.launch

# 다른 환경의 앵커 파일 사용
roslaunch harvest_recorder bringup.launch \
  anchors:=$(rospack find uwb_positioning)/config/anchors/lab.yaml
```

앵커 파일 추가 방법: `catkin_ws/src/uwb_positioning/config/anchors/` 아래에 YAML 파일 추가. 코드 수정 불필요.

---

## 9. 파일 구조

```
harvest_recorder/
  src/
    recorder_node.cpp   # HarvestRecorder 클래스; ApproximateTime 동기화, CSV, publish
    sim_harvest.cpp     # 하드웨어 없이 테스트용 fake publisher
  msg/
    HarvestReading.msg  # Header + UwbPosition + WeightReading
  config/
    params.yaml
  launch/
    bringup.launch      # 전체 시스템
    recorder.launch     # recorder만
  DESIGN.md             # 이 문서
```

---

## 10. 미결 사항

| # | 항목 | 조치 |
|---|------|------|
| 1 | **CSV 스키마 확정** | 다운스트림 소비자가 기대하는 정확한 컬럼·단위 확인 후 헤더와 기록 로직 수정 |
| 2 | **slop 최적값** | 실제 하드웨어 주기 측정 후 `params.yaml` 조정 (`rostopic hz`로 확인) |
| 3 | **position_covariance 활용** | CSV에 공분산 대각 원소를 추가해 측정 신뢰도 기록 고려 |
| 4 | **ExactTime 옵션** | 두 장치의 클락을 하드웨어 동기화할 경우 `ExactTime`으로 전환해 매칭 정확도 향상 가능 |
