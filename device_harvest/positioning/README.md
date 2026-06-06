# UWB 3D Positioning Monitor

ESP32(DW1000)에서 UDP로 전송하는 `RANGE,addr,range,rxpow` 데이터를 수신해
거리/RX를 칼만 필터로 평활화하고, 다중 앵커 삼변측량으로 3D 위치를 추정·시각화합니다.

## 설치

conda (권장):

```bash
conda env create -f environment.yml
conda activate uwb-positioning
```

또는 pip:

```bash
pip install PyQt5 pyqtgraph numpy
```

## 실행

```bash
python main.py            # 기본 3D
python main.py --mode 2d  # 2D 평면 측위 (z 무시)
python main.py --mode 3d  # 3D 측위
```

2D는 맵을 X-Y 평면도 하나로, 3D는 X-Y(평면도) + X-Z(측면도) 두 뷰로 표시합니다.
측위 최소 앵커 수는 2D 3개, 3D 4개로 자동 설정됩니다.

## 모듈 구조

| 파일 | 역할 |
|------|------|
| `config.py` | 모든 설정값 + 앵커 좌표(`ANCHORS`). **여기만 수정하면 됨**. 차원별 좌표/최소앵커 헬퍼 포함 |
| `kalman.py` | 1D 칼만 필터 (검사/갱신 분리, 아웃라이어 게이팅) |
| `multilateration.py` | 가우스-뉴턴 측위(2D/3D 공용) + 위치 좌표 N차원 칼만 |
| `anchor_track.py` | 앵커별 필터·버퍼·그래프 곡선 + 색상 생성 + 타임스탬프 거리 이력 |
| `network.py` | 논블로킹 UDP 수신 + RANGE 파싱 |
| `visualizer_base.py` | 2D/3D 공통: 수신 루프·앵커 관리·시간 동기화 측위 |
| `visualizer_2d.py` | 2D 맵 뷰 (X-Y) |
| `visualizer_3d.py` | 3D 맵 뷰 (X-Y + X-Z) |
| `visualizer.py` | 하위 호환 shim (3D 재노출) |
| `main.py` | 진입점 (`--mode 2d/3d` 선택) |

의존 방향: `main → visualizer_{2d,3d} → visualizer_base → {network, anchor_track, multilateration} → {kalman, config}`
(순수 로직인 `kalman`·`multilateration`·`config`는 PyQt에 의존하지 않음)

## 앵커 추가/변경

`config.py`의 `ANCHORS` 딕셔너리에 줄을 추가/삭제하기만 하면 됩니다.
최소 앵커 수, 색상 배정, 맵 축 범위, 측위 입력 수집이 모두 자동으로 따라갑니다.

```python
ANCHORS = {
    "1A01": (0.0, 0.0, 0.0),   # key = 태그 newRange()의 short address (대문자 HEX)
    "1A02": (5.0, 0.0, 0.0),   # value = (x, y, z) 좌표 (m)
    ...
}
```

- key는 PC로 들어오는 `RANGE` 줄의 두 번째 필드(상대 기기 short address)와 일치해야 합니다.
- 좌표는 항상 (x, y, z) 3D로 적되, **2D 모드에서는 z가 자동으로 무시**됩니다. 좌표표 하나로 2D/3D를 모두 씁니다.
- 좌표 미등록 id가 들어오면 경고만 찍고 측위에서는 제외, 그래프에는 회색으로 표시합니다.
- 3D 안정성을 위해 일부 앵커는 서로 다른 높이(z)에 배치하는 것을 권장합니다.

## 정확도 관련 메모

- 측위 정확도는 입력 거리의 정확도에 달려 있습니다. 먼저 안테나 지연
  캘리브레이션으로 각 거리값 자체를 맞추세요.
- `multilaterate_3d`가 반환하는 잔차(rms)가 크면 거리들이 서로 모순된다는
  뜻으로, NLOS나 이상 앵커를 의심할 수 있습니다.
- **시간 동기화**: 각 거리값은 수신 시각과 함께 기록됩니다. 측위 시 가장 최근
  수신 시각을 기준(t_ref)으로 잡고, 그 시각에서 `SYNC_WINDOW_SEC`(기본 0.3초)
  안에 들어오는 앵커 거리만 모아 계산합니다. 앵커마다 수신 주기가 달라 생기는
  시간 불일치를 줄여, 태그가 움직일 때 위치 오차를 낮춥니다. 앵커 수신율이
  낮아 동기화 창 안의 앵커가 부족하면 창을 늘리되(예: 0.5초), 그만큼 이동 중
  시간 오차가 커지는 절충이 있습니다.
