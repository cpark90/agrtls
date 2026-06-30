# Design flow — mesh-TDMA (decision log)

> Chronological record of how the **mesh-TDMA** design was reached, as
> **Situation → Finding → Decision/pivot**. Terms: [GLOSSARY](GLOSSARY.md). Index: [README](README.md).

Leads to [ARCHITECTURE_mesh_tdma.md](ARCHITECTURE_mesh_tdma.md).

---

## Starting point: multi-anchor priority scheduling request

- **Request**: when several anchors are detected at once, apply a penalty on each update so priority drops, and lower the *measurement* priority of tags that are persistently far or weak.
- **Finding**: the mf-DW1000 TAG sends a **broadcast POLL to all anchors at once** (`transmitPoll(nullptr)`). There is no notion of choosing which anchor to range. A single-poll path exists but its completion logic is hard-wired to a "last index" assumption and is effectively broken.
- **Decision**: real poll scheduling (fork the library) + aging score (fairness) + apply to both TAG and anchor.

## Pivot 1: expand to 10 devices → broadcast proven impossible

- **Request**: support up to 10 devices, with minimal library change and simplicity.
- **Finding (decisive)**: the broadcast RANGE frame uses 17 bytes per device. With `LEN_DATA=90` and `SHORT_MAC_LEN=9`, the max is **4 devices** (the 5th overflows: offset 96 > 90). That is the real reason `MAX_DEVICES=4`. Even a max PHY frame (127 B) only reaches ~6. **10 devices is impossible via broadcast → sequential polling is mandatory.**
- **Pivot**: "sequential polling" becomes a requirement, not an option.

## Pivot 2: topology inversion — one anchor ↔ 5–10 tags

- **New constraint**: a single anchor may detect 5–10 tags (the initiator side, TAG, is the crowded one).
- **Finding**: this is a **multiple-access (collision)** problem, not a scheduling one. mf-DW1000 has no time-multiplexing (CLAUDE.md). The anchor side uses single-device frames, so the 4-device limit does not apply (`MAX_DEVICES` bump → 10 OK). The real bottleneck is RF collisions.
- **Decision**: selective response (anchor skips POLL_ACK for low-priority tags) + a TDMA slot guarantee.

## Pivot 3: single-master polling → multi-master

- **Proposal**: master-polled scheme (anchor polls tags one at a time → zero collision). Clean only with a single anchor.
- **New constraint**: **multiple anchors**, each with 5–10 tags (multi-master). Anchors collide with each other too.
- **Pivot**: move the turn-taking onto tag slots. Proposed **tag-slotted TDMA with a sync beacon** (Tier A, fixed slots).

## Pivot 4: mobility → a single coordinator beacon is invalid

- **New constraint**: no coordinator can manage all tags; as a tag moves, the matched anchor changes.
- **Finding**: a global time reference (single beacon) is itself impossible. For positioning, one tag must be seen by several anchors (handoff).
- **Pivot**: coordinate the **few fixed anchors** instead of the **many moving tags**. Anchor-initiated polling + tags as simple responders (mobility for free).

## Pivot 5: "anchors hold different tag candidates" concern resolved

- **Concern**: to coordinate, an anchor cluster would need to share the same tag-candidate set, but those sets differ between anchors.
- **Resolution (key)**: coordination is about **"interfering anchors must not transmit simultaneously"**, not about sharing a tag list. Each anchor polls only its own tags during its own turn → **no shared tag list needed**. A shared tag is simply ranged by different anchors in different turns. The hard, dynamic part (tag membership) drops out of coordination.
- **Conclusion**: the real object of coordination is the **anchor interference graph**.

## Pivot 6: introduce ESP mesh — split control / measurement planes

- **Question**: can we use the ESP32 mesh?
- **Finding**: DW1000 (UWB) and ESP32 WiFi are separate radios on separate bands → no mutual RF interference or airtime contention. We can split a **control plane (mesh)** from the **measurement plane (UWB)**.
- **Effect**: multi-hop solves "no single coordinator", interference-graph and demand are shared out-of-band, and even hidden terminals can be handled by the scheduler.

## Pivot 7: compute-load check

- **Question**: is mesh + UWB simultaneously feasible on the load?
- **Conclusion**: yes. Separate radios + dual-core (pin tasks) + **TWR timing is done in DW1000 hardware**. The cost of WiFi jitter is not accuracy but **occasional dropped measurements (retried)**. Mitigations: pin cores / prefer ESP-NOW / send in guard intervals / WiFi on anchors only. This repo's `tag_dw1000_accuracy_wifi` variant is prior evidence that coexistence works.

## Pivot 8: multi-hop, distributed agreement, aggregation policy (v1)

- **Decisions**: (1) building-wide multi-hop (ESP-WIFI-MESH); (2) anchor distributed agreement via **multi-agent theory**; (3) coordination-only first, aggregation in mind.
- **Formalization**: slot assignment as a **DCOP** (variable = slot, constraint = neighbors differ, objective = reuse + demand-proportional). Algorithms: DRAND-style baseline + MGM/DSA rebalancing + gossip consensus.

## Pivot 9: anchor power is also variable → v2 (rootless, lease-based)

- **New constraint**: anchor power toggles (on/off/return). A root is not required initially; add it later for aggregation.
- **Pivot**: go **fully distributed, self-healing, lease-based**. The esp_mesh root is auto-elected (internal); the backend gateway is deferred. With "interference geometry fixed + node presence dynamic", churn is handled by lease expiry / re-acquisition.

## Pivot 10: anchor location is also variable → v3 (fully dynamic MANET)

- **New constraint**: anchor location can change.
- **Pivot**: drop the "fixed geometry" assumption. Both nodes and edges are dynamic (a MANET). L2 does continuous neighbor sensing. **Regime-adaptive MAC**: structured TDMA (collision-free) when slow, contention (CSMA) fallback when fast. The decisive variable is the **mobility timescale**.

## Pivot 11: occasional relocation + P3-centric → v4 (current)

- **Decisions**: (1) anchors are **occasionally relocated** (slow change); (2) **P3-centric** (Foundation as prerequisite + body = P3); (3) **channels used together** (color = slot × channel); (4) **MGM confirmed**.
- **Simplifications**: collision-free guarantee returns as the **primary mode**; contention is demoted to a relocation-transient safety net. Leases are long, re-coloring is event-triggered, sensing is low-rate.
- **Output**: [`ARCHITECTURE_mesh_tdma.md`](./ARCHITECTURE_mesh_tdma.md) v4 and [`DESIGN_P3_core_mgm.md`](./DESIGN_P3_core_mgm.md).

---

## Key takeaways

1. The broadcast 4-device frame limit makes sequential polling mandatory.
2. The object of coordination is the **anchor interference graph**, not a tag list.
3. Mobility, power, and location variability all concentrate on the **anchors** → keep tags as simple responders throughout.
4. Splitting UWB (measurement) from WiFi (coordination) resolves collisions, load, and the no-coordinator problem at once.
5. TWR accuracy needs no clock sync → the mesh only handles ms-level slot alignment.
6. Occasional (slow) relocation is what makes a structural collision-free TDMA feasible.
