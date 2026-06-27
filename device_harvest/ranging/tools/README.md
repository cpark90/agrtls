# tools/ — 로그 분석 / 시각화

펌웨어가 내보내는 CSV(`deviceId,range_m,rxPower_dBm,timestamp_ms,nlosFlag`)를
실시간으로 받아 시각화한다. 시리얼(USB)과 UDP(WiFi) 두 입력을 모두 지원.

## 설치

PlatformIO와 같은 conda 환경을 쓰거나 별도 환경에 설치한다:

```bash
conda activate platformio        # 또는 원하는 환경
pip install -r tools/requirements.txt
```

## 실시간 그래프 (realtime_plot.py)

거리(상단)와 RX Power(하단)를 디바이스별 선그래프로 표시. NLOS 의심 지점은
거리 그래프에 빨간 X로 표시된다.

### 시리얼(USB) — 보드를 PC에 직접 연결한 경우
```bash
# 포트 자동 추정
python tools/realtime_plot.py serial

# 포트 직접 지정 (pio device list 로 확인)
python tools/realtime_plot.py serial --port /dev/ttyUSB0 --baud 115200
```

주의: 시리얼 포트는 한 번에 하나의 프로그램만 점유 가능.
`pio device monitor`를 켜둔 채로는 이 스크립트가 포트를 못 연다. 모니터를 끄고 실행할 것.

### UDP(WiFi) — tag_*_wifi 변종이 송출하는 경우
```bash
# 펌웨어의 SERVER_PORT 와 동일하게 (기본 9000)
python tools/realtime_plot.py udp --host 0.0.0.0 --port 9000
```
펌웨어 쪽 SERVER_IP를 이 PC의 IP로 맞춰야 데이터가 도착한다.

### 공통 옵션
- `--window N`   : 표시할 최근 샘플 수 (기본 200)
- `--device ID`  : 특정 deviceId만 표시 (여러 번 지정 가능)
  예) `--device TAG_01 --device TAG_02`

## 동작 확인용 더미 송신 (보드 없이 테스트)

UDP 모드를 보드 없이 테스트하려면 다른 터미널에서:
```bash
python tools/fake_sender.py --port 9000
```
