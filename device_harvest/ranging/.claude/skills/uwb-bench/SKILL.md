---
name: uwb-bench
description: >
  Build/upload UWB nodes and read their serial output on this hardware bench.
  Use for the Makerfabs ESP32 UWB (DW1000) ranging firmware whenever you need to
  flash an anchor/tag/responder to a /dev/ttyUSB* port, capture serial without an
  interactive TTY (pio device monitor fails headless), tally per-device ranges to
  check clustering / link quality, or run the host unit tests. Multi-node window-TDMA
  validation (anchor_dw1000_window + tag_dw1000_responder).
---

# uwb-bench

Frequently-used commands for developing and validating the window-TDMA UWB firmware.
All paths are relative to this skill dir (`.claude/skills/uwb-bench/`); the scripts
locate the project root (the dir with `platformio.ini`) themselves.

## Bench layout (typical)

| port | node |
|------|------|
| /dev/ttyUSB0 | anchor A0 (`anchor_dw1000_window`, `-DANCHOR_ID=0`) |
| /dev/ttyUSB1 | anchor A1 (`-DANCHOR_ID=1`) |
| /dev/ttyUSB2 | tag T0 (`tag_dw1000_responder`, `-DTAG_ID=0`) |
| /dev/ttyUSB3 | tag T1 (`-DTAG_ID=1`) |

## Flash a node

```bash
scripts/flash.sh anchor-window 0 /dev/ttyUSB0   # A0
scripts/flash.sh anchor-window 1 /dev/ttyUSB1   # A1
scripts/flash.sh responder     0 /dev/ttyUSB2   # T0
scripts/flash.sh responder     1 /dev/ttyUSB3   # T1
```
roles: `anchor-window | responder | anchor-acc | tag-acc | anchor-mesh`.
Injects `-DANCHOR_ID`/`-DTAG_ID` via `PLATFORMIO_BUILD_FLAGS` and uploads to the port.

## Read serial (headless)

`pio device monitor` needs an interactive TTY and fails here
(`termios.error: Inappropriate ioctl for device`). Use pyserial instead:

```bash
# anchor schedule dump + range logs (opening the port resets the ESP32 -> allow convergence)
python3 scripts/serial_capture.py /dev/ttyUSB0 -d 18 -g '^# A|^[AT][0-9],'
# raw, everything
python3 scripts/serial_capture.py /dev/ttyUSB2 -d 16
```

## Tally per-device ranges (clustering / link check)

```bash
# how often / how strongly each anchor reaches tag T0 (look for >1 anchor = clustering)
python3 scripts/tally.py /dev/ttyUSB2 -d 20
```
Reports per-id count + RXP mean/min/max and `# +dev` / `# -dev` events.

## Host unit tests (no radio)

```bash
g++ -std=c++17 -I src/common tools/host_test/test_window_modules.cpp -o /tmp/tw && /tmp/tw
g++ -std=c++17 -I src/common tools/host_test/test_p3_modules.cpp    -o /tmp/tp && /tmp/tp
```

## What to look for (window-TDMA validation)

- Anchor status line: `# A{id} win=k/n disc= ranges= sched: T0=0 T1=c` — `sched:` must
  **match across anchors** (shared registry -> same coloring); `c` = candidate (far/weak).
- A tag (`tally.py` on its port) should be ranged by **multiple anchors** close in time
  (clustered ranges, needed for localization).
- `# -dev` repeating for one anchor = that anchor's exchanges with the tag are failing
  (weak link or slot collision / epoch not synced).
