#!/usr/bin/env python3
"""Non-TTY serial capture for UWB nodes.

`pio device monitor` (miniterm) needs an interactive TTY, which is unavailable in
non-interactive shells (it fails with `termios.error: Inappropriate ioctl for device`).
This reads the serial port with pyserial instead, so it works headless.

Usage:
  python3 serial_capture.py PORT [-b BAUD] [-d SECONDS] [-g REGEX]
Examples:
  python3 serial_capture.py /dev/ttyUSB0 -d 18 -g '^# A|^[AT][0-9],'
  python3 serial_capture.py /dev/ttyUSB2 -d 16

Note: opening the port toggles DTR, so the ESP32 resets. Allow a few seconds for
discovery / frame-color convergence before reading steady-state lines.
"""
import argparse
import re
import time

import serial


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("-b", "--baud", type=int, default=115200)
    ap.add_argument("-d", "--duration", type=float, default=18)
    ap.add_argument("-g", "--grep", default=None,
                    help="regex filter (matched with re.search)")
    args = ap.parse_args()

    pat = re.compile(args.grep) if args.grep else None
    s = serial.Serial(args.port, args.baud, timeout=1)
    t = time.time()
    while time.time() - t < args.duration:
        line = s.readline().decode("utf-8", "replace").rstrip()
        if not line:
            continue
        if pat is None or pat.search(line):
            print(line, flush=True)


if __name__ == "__main__":
    main()
