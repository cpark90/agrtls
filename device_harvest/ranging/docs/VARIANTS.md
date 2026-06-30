# Variant catalog

Single source of truth for the firmware variants. Update this table whenever a variant is added or
changed. Terms: [GLOSSARY.md](GLOSSARY.md). Start here: [README.md](README.md).

A **variant** = an independent source folder + an independent PlatformIO env. The folder name, the env
name, and the variant name are always identical.

## Naming

```
{role}_{chip}_{rfmode}[_{features...}]
```
- `role`   : `anchor` | `tag`
- `chip`   : `dw1000` | `dw3000`
- `rfmode` : `accuracy` | `lowpower` | `fast`
- `features` (optional, multiple): `oled` | `wifi` | `filtered` …

## Current variants

| Variant (env) | role | chip | rfmode | model | Purpose |
|---|---|---|---|---|---|
| `anchor_dw1000_accuracy` | anchor | DW1000 | accuracy | native | Native anchor = **responder**. Address: `-D ANCHOR_ID=n` |
| `tag_dw1000_accuracy` | tag | DW1000 | accuracy | native | Native tag = **initiator** (broadcast POLL). Address: `-D TAG_ID=n` |
| `anchor_dw1000_synchronous` | anchor | DW1000 | accuracy | synchronous TDMA | **Current** anchor = initiator: shared registry → frame coloring → poll the frame's tag. `-D ANCHOR_ID=n` |
| `tag_dw1000_responder` | tag | DW1000 | accuracy | frame/distributed TDMA | Mobile tag = **responder** (replies only). Shared by both TDMA models. `-D TAG_ID=n` |
| `anchor_dw1000_accuracy_meshagent` | anchor | DW1000 | accuracy | distributed TDMA | Anchor = initiator that schedule-polls its own tags; MGM slot negotiation. Self-contained variant. `-D ANCHOR_ID=n` |
| `anchor_dw3000_accuracy` | anchor | DW3000 | accuracy | — | DW3000 anchor (skeleton) |
| `tag_dw3000_fast_filtered` | tag | DW3000 | fast | — | DW3000 fast + filtered tag (skeleton) |

## Role inversion — native vs the TDMA models

The radio role (`_type`) of a physical anchor/tag flips between the families (see GLOSSARY: *role
inversion*):

| Family | Physical tag | Physical anchor |
|---|---|---|
| **native** (`*_dw1000_accuracy`) | initiator (`startAsTag`, broadcast POLL) | responder (`startAsAnchor`) |
| **TDMA** (`*_meshagent`, `*_window` / `*_responder`) | responder (`startAsResponder`) | initiator (`startAsInitiator`, scheduled poll) |

- **native**: stock library broadcast TWR. ≤4 anchors per tag (broadcast RANGE frame limit). No mesh,
  no scheduling. Library loop: `loop()`.
- **TDMA**: role inversion + scheduled polling (`setScheduledMode(true)` + `pollDevice()`). Library
  loops: `loopInitiator()`/`loopResponder()` (synchronous TDMA), `loopMeshagent()` (distributed TDMA).

`startAsInitiator()`/`startAsResponder()` are readability aliases of `startAsTag()`/`startAsAnchor()`
(identical behavior). Designs: [ARCHITECTURE_synchronous_tdma.md](ARCHITECTURE_synchronous_tdma.md) (authoritative,
current) and [ARCHITECTURE_distributed_tdma.md](ARCHITECTURE_distributed_tdma.md) (earlier model).

## Adding a variant

1. Create `src/{variant}/main.cpp` (copy the nearest existing variant to start).
2. Add `[env:{variant}]` to `platformio.ini`:
   - `extends = chip_dw1000` (or `chip_dw3000`)
   - `build_src_filter`: `+<common/> +<common_{chip}/> +<{variant}/>`
   - `build_flags`: `-D ROLE_{ANCHOR|TAG}`, `-D RFMODE_{...}`, plus any feature macros
   - extra libraries (OLED, …): append to that env's `lib_deps`
3. Add a row to the table above.
4. Build: `pio run -e {variant}`.

## Short-address assignment

On `startAsAnchor/Tag(addr, mode, false)`, the short address derives from the first two bytes of the
EUI-64: `short_addr = byte[1]*256 + byte[0]`. Fixing `byte[1] = 0x00` gives `short_addr = byte[0]`.

```
EUI-64: "BB:00:XX:XX:XX:XX:XX:XX"
         └┘ └┘
         │  fixed 0x00
         └─ short-address byte (BB)
```

| Role | byte[0] range | short addr | label | EUI-64 example |
|------|---------------|------------|-------|----------------|
| Anchor n | `0x00 + n` (0x00–0x3F) | n (0–63) | `"A{n}"` | `"0n:00:5B:D5:A9:9A:E2:9C"` |
| Tag n | `0x80 + n` (0x80–0xBF) | 128+n (128–191) | `"T{n}"` | `"(80+n):00:22:EA:82:60:3B:9C"` |

Per-board number is injected at build time (`-D ANCHOR_ID=n` / `-D TAG_ID=n`) for multi-node tests
(A0/A1/…, T0/T1/…).

### logRange deviceId

`newRange()` converts the distant device's short address with `shortAddrToId()`:

```cpp
char devId[8];
shortAddrToId(d->getShortAddress(), devId, sizeof(devId));
logRange(devId, d->getRange(), d->getRXPower());
```

- called on an **anchor** → distant device = tag → `"T0"`, `"T1"`, …
- called on a **tag** → distant device = anchor → `"A0"`, `"A1"`, …

`shortAddrToId()` is in `src/common/logging.h`.

## Chip notes

- **DW1000**: high-level `DW1000Ranging` callback API. `rf_config_dw1000.h` + `applyRfConfigDW1000()`.
- **DW3000**: different (low-level `dwt_*`) API, no 110 kbps, channels 5/9, `rf_config_dw3000.h`.
  `main.cpp` is a skeleton — fill it with the real library.

## Module layout

Shared, chip-independent (`src/common/`):

- `logging.h` — CSV output format (all variants; fixed field order)
- `range_filter.h` — EMA + outlier filter (filtered variants)
- `mesh_link.h` — ESP-NOW control-plane transport (generic byte frame, `MESH_LINK_MAX_FRAME`)
- `mesh_wire.h` — shared wire utils: little-endian cursors + `meshMsgType` + `SYNC` (both TDMA models)
- `mesh_msg.h` — **synchronous TDMA** message: `TAGINFO` (shared-registry exchange)
- `tag_registry.h` / `tag_quality.h` / `frame_color.h` / `frame_schedule.h` — synchronous TDMA scheduling

Per-variant (self-contained), in `src/anchor_dw1000_accuracy_meshagent/`:

- `superframe.h`, `peer_scheduler.h`, `interference.h`, `mgm_agent.h`, `mgm_msg.h`
  (VALUE/GAIN/TAGLIST/AUDIBLE) — **meshagent-only**, so kept in the variant folder, not `common`.
  Firmware resolves them via same-folder quoted includes (no extra `-I`); the host test needs
  `-I src/anchor_dw1000_accuracy_meshagent`.
