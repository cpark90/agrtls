#!/usr/bin/env python3
"""
realtime_plot.py - UWB 거리/RX Power 실시간 시각화

펌웨어가 내보내는 CSV 한 줄 포맷을 받아 실시간 선그래프로 표시한다.
  포맷: deviceId,range_m,rxPower_dBm,timestamp_ms
  (logging.h / tag_*_wifi 의 sendUdp 포맷과 동일. 과거 nlosFlag 필드는 선택적으로 무시)

입력 소스 두 가지 지원:
  - serial : 보드를 USB로 연결한 경우 (pyserial)
  - udp    : tag_*_wifi 변종이 UDP로 송출하는 경우

사용 예:
  # 시리얼 (포트 자동 추정 또는 명시)
  python realtime_plot.py serial --port /dev/ttyUSB0 --baud 115200
  python realtime_plot.py serial            # --port 생략 시 첫 후보 자동 선택

  # UDP (펌웨어의 SERVER_PORT와 동일하게)
  python realtime_plot.py udp --host 0.0.0.0 --port 9000

  # 디바이스 여러 개면 각각 다른 선으로 그려진다 (deviceId 기준).
  # 윈도우 크기(최근 N개 점)와 그릴 디바이스 필터도 옵션으로 지정 가능.

의존성:
  pip install matplotlib pyserial
"""

import argparse
import collections
import socket
import sys
import threading
import time

try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
except ImportError:
    sys.exit("matplotlib is required:  pip install matplotlib")


# ---------------------------------------------------------------------------
# 공통: CSV 한 줄 파싱
# ---------------------------------------------------------------------------
def parse_line(line):
    """CSV 한 줄 -> dict 또는 None.
    deviceId,range_m,rxPower_dBm,timestamp_ms[,nlosFlag]
    '#'로 시작하는 주석/상태 줄은 무시.
    """
    line = line.strip()
    if not line or line.startswith("#"):
        return None
    parts = line.split(",")
    if len(parts) < 3:
        return None
    try:
        rec = {
            "device": parts[0],
            "range": float(parts[1]),
            "rxp": float(parts[2]),
            "ts": int(parts[3]) if len(parts) > 3 else int(time.time() * 1000),
            "nlos": int(parts[4]) if len(parts) > 4 else 0,
        }
    except ValueError:
        return None
    return rec


# ---------------------------------------------------------------------------
# 입력 소스 (백그라운드 스레드에서 큐로 레코드 전달)
# ---------------------------------------------------------------------------
class SerialSource(threading.Thread):
    def __init__(self, port, baud, out_queue):
        super().__init__(daemon=True)
        self.port, self.baud, self.q = port, baud, out_queue
        self._stop = threading.Event()

    def run(self):
        try:
            import serial  # pyserial
        except ImportError:
            print("pyserial is required:  pip install pyserial", file=sys.stderr)
            return
        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
        except Exception as e:
            print(f"Failed to open serial port ({self.port}): {e}", file=sys.stderr)
            return
        print(f"[serial] {self.port} @ {self.baud} listening")
        while not self._stop.is_set():
            try:
                raw = ser.readline().decode("utf-8", errors="ignore")
            except Exception:
                continue
            rec = parse_line(raw)
            if rec:
                self.q.append(rec)
        ser.close()

    def stop(self):
        self._stop.set()


class UdpSource(threading.Thread):
    def __init__(self, host, port, out_queue):
        super().__init__(daemon=True)
        self.host, self.port, self.q = host, port, out_queue
        self._stop = threading.Event()

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((self.host, self.port))
        sock.settimeout(1.0)
        print(f"[udp] {self.host}:{self.port} listening")
        while not self._stop.is_set():
            try:
                data, _ = sock.recvfrom(256)
            except socket.timeout:
                continue
            except Exception:
                continue
            rec = parse_line(data.decode("utf-8", errors="ignore"))
            if rec:
                self.q.append(rec)
        sock.close()

    def stop(self):
        self._stop.set()


def autodetect_serial_port():
    """가장 그럴듯한 시리얼 포트 하나 추정."""
    try:
        from serial.tools import list_ports
    except ImportError:
        return None
    candidates = []
    for p in list_ports.comports():
        name = (p.device or "")
        desc = (p.description or "").lower()
        # ESP32 보드에서 흔한 USB-시리얼 칩
        if any(k in desc for k in ("cp210", "ch340", "uart", "usb")) or \
           name.startswith(("/dev/ttyusb", "/dev/ttyacm", "/dev/cu.")):
            candidates.append(p.device)
    return candidates[0] if candidates else None


# ---------------------------------------------------------------------------
# 시각화: 디바이스별 거리/RX Power 선그래프 (위/아래 2단)
# ---------------------------------------------------------------------------
class LivePlot:
    def __init__(self, window=200, only_devices=None):
        self.window = window
        self.only = set(only_devices) if only_devices else None
        # device -> deque of (ts, range, rxp, nlos)
        self.data = collections.defaultdict(
            lambda: collections.deque(maxlen=self.window)
        )
        self.queue = collections.deque()  # 소스 스레드가 append, 메인이 소비

        self.fig, (self.ax_r, self.ax_p) = plt.subplots(
            2, 1, figsize=(10, 7), sharex=True
        )
        self.fig.suptitle("UWB Real-time Monitor (Range / RX Power)")
        self.ax_r.set_ylabel("Range (m)")
        self.ax_r.grid(True, alpha=0.3)
        self.ax_p.set_ylabel("RX Power (dBm)")
        self.ax_p.set_xlabel("Recent samples")
        self.ax_p.grid(True, alpha=0.3)
        self.lines_r = {}
        self.lines_p = {}
        self.nlos_scatter = None

    def _drain_queue(self):
        # 소스 스레드가 쌓아둔 레코드를 디바이스별 버퍼로 이동
        while self.queue:
            rec = self.queue.popleft()
            if self.only and rec["device"] not in self.only:
                continue
            self.data[rec["device"]].append(
                (rec["ts"], rec["range"], rec["rxp"], rec["nlos"])
            )

    def _ensure_lines(self, device):
        if device not in self.lines_r:
            (lr,) = self.ax_r.plot([], [], label=device, linewidth=1.4)
            (lp,) = self.ax_p.plot([], [], label=device, linewidth=1.4)
            self.lines_r[device] = lr
            self.lines_p[device] = lp
            self.ax_r.legend(loc="upper right", fontsize=8)
            self.ax_p.legend(loc="upper right", fontsize=8)

    def update(self, _frame):
        self._drain_queue()
        all_r, all_p = [], []
        nlos_x, nlos_y = [], []
        for device, buf in self.data.items():
            if not buf:
                continue
            self._ensure_lines(device)
            xs = list(range(len(buf)))
            ranges = [b[1] for b in buf]
            rxps = [b[2] for b in buf]
            self.lines_r[device].set_data(xs, ranges)
            self.lines_p[device].set_data(xs, rxps)
            all_r += ranges
            all_p += rxps
            # NLOS 의심 지점을 거리 그래프에 표시
            for i, b in enumerate(buf):
                if b[3]:
                    nlos_x.append(i)
                    nlos_y.append(b[1])

        # 축 범위 자동 조정
        if all_r:
            self.ax_r.set_xlim(0, self.window)
            rmin, rmax = min(all_r), max(all_r)
            pad = max(0.1, (rmax - rmin) * 0.1)
            self.ax_r.set_ylim(rmin - pad, rmax + pad)
        if all_p:
            self.ax_p.set_xlim(0, self.window)
            pmin, pmax = min(all_p), max(all_p)
            pad = max(1.0, (pmax - pmin) * 0.1)
            self.ax_p.set_ylim(pmin - pad, pmax + pad)

        # NLOS 마커 갱신 (거리축)
        if self.nlos_scatter is not None:
            self.nlos_scatter.remove()
            self.nlos_scatter = None
        if nlos_x:
            self.nlos_scatter = self.ax_r.scatter(
                nlos_x, nlos_y, c="red", s=18, marker="x",
                label="_nlos", zorder=5
            )
        return []

    def run(self):
        self._ani = animation.FuncAnimation(
            self.fig, self.update, interval=100, blit=False, cache_frame_data=False
        )
        plt.tight_layout()
        plt.show()


# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description="UWB Range/RX Power real-time visualizer")
    sub = ap.add_subparsers(dest="source", required=True)

    sp = sub.add_parser("serial", help="read from USB serial")
    sp.add_argument("--port", default=None, help="serial port (auto-detected if omitted)")
    sp.add_argument("--baud", type=int, default=115200)

    up = sub.add_parser("udp", help="receive over UDP")
    up.add_argument("--host", default="0.0.0.0")
    up.add_argument("--port", type=int, default=9000)

    for p in (sp, up):
        p.add_argument("--window", type=int, default=200, help="number of recent samples to display")
        p.add_argument("--device", action="append", default=None,
                       help="show only this deviceId (can be specified multiple times)")

    args = ap.parse_args()

    plot = LivePlot(window=args.window, only_devices=args.device)

    if args.source == "serial":
        port = args.port or autodetect_serial_port()
        if not port:
            sys.exit("No serial port found. Specify one with --port. "
                     "(use 'pio device list' to check)")
        src = SerialSource(port, args.baud, plot.queue)
    else:
        src = UdpSource(args.host, args.port, plot.queue)

    src.start()
    try:
        plot.run()
    except KeyboardInterrupt:
        pass
    finally:
        src.stop()


if __name__ == "__main__":
    main()
