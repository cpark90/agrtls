#!/usr/bin/env python3
"""Tally UWB range CSV lines per device over a live serial capture.

Counts logRange lines `devId,range_m,rxp_dBm,ts_ms,nlos` per device id and reports
count + RXP stats, plus `# +dev` / `# -dev` discovery events. Useful to check
clustering / which anchors actually reach a tag (or which tags an anchor ranges).

Usage:
  python3 tally.py PORT [-b BAUD] [-d SECONDS]
Example:
  python3 tally.py /dev/ttyUSB2 -d 20   # which anchors range tag T0, how often
"""
import argparse
import time
from collections import Counter

import serial


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("-b", "--baud", type=int, default=115200)
    ap.add_argument("-d", "--duration", type=float, default=20)
    args = ap.parse_args()

    s = serial.Serial(args.port, args.baud, timeout=1)
    counts = Counter()
    rxp = {}
    events = Counter()
    t = time.time()
    while time.time() - t < args.duration:
        l = s.readline().decode("utf-8", "replace").rstrip()
        if not l:
            continue
        if "+dev" in l or "-dev" in l:
            events[l] += 1
            continue
        f = l.split(",")
        if len(f) >= 3 and f[0] and f[0][0] in "ATat":
            counts[f[0]] += 1
            try:
                rxp.setdefault(f[0], []).append(float(f[2]))
            except ValueError:
                pass

    print("counts:", dict(counts))
    for k, v in sorted(rxp.items()):
        print(f"  {k}: n={len(v)} rxp mean={sum(v)/len(v):.1f} "
              f"min={min(v):.1f} max={max(v):.1f}")
    if events:
        print("dev events:", dict(events))


if __name__ == "__main__":
    main()
