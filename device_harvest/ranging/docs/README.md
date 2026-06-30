# Ranging firmware — documentation index

Makerfabs ESP32 + DW1000 UWB ranging firmware for multi-anchor / multi-tag localization.
This folder is the design documentation; start here.

> All terms (anchor, tag, initiator/responder, window, slot, MGM, far/weak, …) are defined once in
> **[GLOSSARY.md](GLOSSARY.md)** — read it first if a word is unclear.

## Two TDMA models (don't mix them up)

The firmware has **two independent coordination models**, plus the stock broadcast mode:

| Model | What it does | Variants | Status |
|---|---|---|---|
| **window-TDMA** | All anchors range the *same* tag per window → ranges are time-clustered for localization. Deterministic coloring from a shared registry. | `anchor_dw1000_window`, `tag_dw1000_responder` | **current / authoritative** |
| **mesh-TDMA** | Each anchor schedules its *own* tags; anchors negotiate slots via MGM over the mesh. | `anchor_dw1000_accuracy_meshagent` | earlier model (kept, self-contained) |
| **native broadcast** | Stock mf-DW1000 broadcast TWR, no scheduling. | `anchor_dw1000_accuracy`, `tag_dw1000_accuracy` | reference / bring-up |

Role note: in the TDMA models the physical **anchor is the radio initiator** and the physical **tag is
the radio responder** (role inversion) — see GLOSSARY.

## Reading order

1. **[GLOSSARY.md](GLOSSARY.md)** — terminology.
2. **[SYSTEM_OVERVIEW.md](SYSTEM_OVERVIEW.md)** — entities, roles, the relationship graphs, the big picture.
3. **[VARIANTS.md](VARIANTS.md)** — the firmware variant catalog (folder = env = variant) + module layout.
4. window-TDMA (current):
   - **[ARCHITECTURE_window_tdma.md](ARCHITECTURE_window_tdma.md)** — authoritative design + HW-validated layers.
5. mesh-TDMA (earlier model):
   - **[ARCHITECTURE_mesh_tdma.md](ARCHITECTURE_mesh_tdma.md)** — design overview.
   - **[DESIGN_FLOW_mesh_tdma.md](DESIGN_FLOW_mesh_tdma.md)** — decision log (how the design got here).
   - **[DESIGN_P3_scope1.md](DESIGN_P3_scope1.md)** — deep design (single-channel single-slot MGM + lease + scheduler).
   - **[DESIGN_P3_core_mgm.md](DESIGN_P3_core_mgm.md)** — the MGM coordination algorithm in detail.
   - **[DESIGN_P3_modules.md](DESIGN_P3_modules.md)** — per-module API of the meshagent modules.

## Conventions

- Documents in English; domain terms stay English (`CLAUDE.md`).
- `ARCHITECTURE_window_tdma.md` is the **authoritative** source for the current model; if a doc
  disagrees with it, it wins.
- `VARIANTS.md` is the single source of truth for the variant list.
