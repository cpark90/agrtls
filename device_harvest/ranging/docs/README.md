# Ranging firmware — documentation index

Makerfabs ESP32 + DW1000 UWB ranging firmware for multi-anchor / multi-tag localization.
This folder is the design documentation; start here.

> Standard terms (anchor, tag, initiator/responder, TWR/DS-TWR, frame, slot, NLOS, MGM, …) are defined
> once in **[GLOSSARY.md](GLOSSARY.md)** — read it first if a word is unclear. Project-specific design
> terms are defined in the document that uses them.

## Two TDMA models (don't mix them up)

The firmware has **two independent coordination models**, plus the stock broadcast mode:

| Model | What it does | Variants | Status |
|---|---|---|---|
| **synchronous TDMA** | All anchors range the *same* tag per frame → ranges are time-clustered for localization. Deterministic coloring from a shared registry. | `anchor_dw1000_synchronous`, `tag_dw1000_responder` | **current / authoritative** |
| **distributed TDMA** | Each anchor schedules its *own* tags; anchors negotiate slots via MGM over the mesh. | `anchor_dw1000_accuracy_meshagent` | earlier model (kept, self-contained) |
| **native broadcast** | Stock mf-DW1000 broadcast TWR, no scheduling. | `anchor_dw1000_accuracy`, `tag_dw1000_accuracy` | reference / bring-up |

Role note: in the TDMA models the physical **anchor is the radio initiator** and the physical **tag is
the radio responder** (role inversion) — see [VARIANTS.md](VARIANTS.md).

## Reading order

1. **[GLOSSARY.md](GLOSSARY.md)** — terminology.
2. **[SYSTEM_OVERVIEW.md](SYSTEM_OVERVIEW.md)** — entities, roles, the relationship graphs, the big picture.
3. **[VARIANTS.md](VARIANTS.md)** — the firmware variant catalog (folder = env = variant) + module layout.
4. synchronous TDMA (current):
   - **[ARCHITECTURE_synchronous_tdma.md](ARCHITECTURE_synchronous_tdma.md)** — authoritative design + HW-validated layers.
5. distributed TDMA (earlier model):
   - **[ARCHITECTURE_distributed_tdma.md](ARCHITECTURE_distributed_tdma.md)** — design overview.
   - **[DESIGN_FLOW_distributed_tdma.md](DESIGN_FLOW_distributed_tdma.md)** — decision log (how the design got here).
   - **[DESIGN_distributed_tdma_core.md](DESIGN_distributed_tdma_core.md)** — deep design (single-channel single-slot MGM + lease + scheduler).
   - **[DESIGN_distributed_tdma_mgm.md](DESIGN_distributed_tdma_mgm.md)** — the MGM coordination algorithm in detail.
   - **[DESIGN_distributed_tdma_modules.md](DESIGN_distributed_tdma_modules.md)** — per-module API of the meshagent modules.

## Conventions

- Documents in English; domain terms stay English (`CLAUDE.md`).
- `ARCHITECTURE_synchronous_tdma.md` is the **authoritative** source for the current model; if a doc
  disagrees with it, it wins.
- `VARIANTS.md` is the single source of truth for the variant list.
