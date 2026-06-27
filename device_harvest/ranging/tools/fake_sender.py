#!/usr/bin/env python3
"""
fake_sender.py - 보드 없이 realtime_plot.py 를 테스트하기 위한 더미 데이터 송신기.

펌웨어 CSV 포맷과 동일한 줄을 UDP로 쏜다. 거리는 천천히 변하는 사인파 +
가끔 멀티패스/NLOS를 흉내 낸 스파이크를 섞는다.

사용:
  python fake_sender.py --port 9000 --host 127.0.0.1
그리고 다른 터미널에서:
  python realtime_plot.py udp --port 9000
"""

import argparse
import math
import random
import socket
import time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=9000)
    ap.add_argument("--rate", type=float, default=20.0, help="sends per second")
    ap.add_argument("--devices", type=int, default=2, help="number of devices to simulate")
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    period = 1.0 / args.rate
    t0 = time.time()
    print(f"[fake] -> {args.host}:{args.port}  ({args.rate}Hz, {args.devices} devices)")

    try:
        while True:
            now = time.time()
            t = now - t0
            for i in range(args.devices):
                base = 2.0 + i * 1.5
                rng = base + 0.6 * math.sin(t * 0.8 + i)
                rxp = -70.0 - 8.0 * math.sin(t * 0.5 + i) + random.gauss(0, 0.6)
                nlos = 0
                # 5% 확률로 NLOS 스파이크 (거리 튐 + RX Power 급감)
                if random.random() < 0.05:
                    rng += random.uniform(0.4, 1.2)
                    rxp -= random.uniform(8, 16)
                    nlos = 1
                rng += random.gauss(0, 0.03)  # 평상시 측정 노이즈
                msg = f"TAG_{i+1:02d},{rng:.3f},{rxp:.2f},{int(now*1000)},{nlos}"
                sock.sendto(msg.encode(), (args.host, args.port))
            time.sleep(period)
    except KeyboardInterrupt:
        print("\n[fake] stopped")


if __name__ == "__main__":
    main()
