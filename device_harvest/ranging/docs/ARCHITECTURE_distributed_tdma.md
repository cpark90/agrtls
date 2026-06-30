# Architecture — distributed TDMA

> The **earlier** model (the `anchor_dw1000_accuracy_meshagent` variant): each anchor schedules its own
> tags and anchors negotiate slots via MGM over the mesh. Superseded by synchronous TDMA for clustering, but
> kept. Terms: [GLOSSARY](GLOSSARY.md). Index: [README](README.md).

Consolidated design for **collision-free coordination of UWB distance measurement** in a multi-anchor /
multi-tag environment. How the decisions were reached: [DESIGN_FLOW_distributed_tdma.md](DESIGN_FLOW_distributed_tdma.md).
Detailed coordination-layer (L3/L4) design: [DESIGN_distributed_tdma_mgm.md](DESIGN_distributed_tdma_mgm.md).

---

## 1. Terminology (read this first)

| Term | Meaning |
|---|---|
| **Anchor** | A node that *initiates* UWB polling and participates in the WiFi mesh. Powered, fixed location but **occasionally relocated**, power may toggle on/off. |
| **Tag** | A *mobile* node that only *responds* to UWB polls. Does **not** join the mesh. Treated as near-"dumb". |
| **Control plane** | The WiFi (ESP-MESH) network used only for coordination messages between anchors. |
| **Measurement plane** | The UWB (DW1000) radio used for actual ranging (Two-Way Ranging). |
| **TWR** | Two-Way Ranging. Distance from round-trip time; **does not require clock sync** for accuracy. |
| **Superframe** | The repeating TDMA period, divided into `K_s` time **slots**. |
| **Slot** | One time division of the superframe. An anchor transmits only in slots it owns. |
| **Channel** | A UWB radio channel (DW1000 supports 1/2/3/5). |
| **Color** | A `(slot, channel)` pair. Two anchors with the same color collide *iff* they interfere. |
| **Interference graph** | Graph over anchors; an edge means "if both transmit at the same color, they collide". |
| **Demand `d_i`** | How much airtime anchor *i* needs, = weighted count of its tags (priority-aware). |
| **Lease** | A time-bounded claim on a color, periodically renewed; auto-released on power loss. |
| **MGM** | Maximum-Gain Messaging — the distributed local-search algorithm used for color assignment. |
| **DCOP** | Distributed Constraint Optimization Problem — the formal frame for the assignment. |
| **gossip** | Local peer-to-peer averaging used for loose (ms-level) slot-phase agreement. |

## 2. Purpose / Scope

- **Purpose**: When several anchors each range 5–10 mobile tags, avoid radio collisions *structurally* while honoring priority (far/weak tags measured less often).
- **Scope (now)**: the **coordination/MAC layer** only. Result aggregation and positioning (EKF) are external/later.
- **Out of scope**: absolute position estimation of anchors/tags belongs to the external positioning system (per CLAUDE.md). This design uses only **relative interference relations**.

## 3. Confirmed Environment Constraints

| Item | Value |
|---|---|
| Topology | **Multiple anchors**, 5–10 tags per anchor (partially overlapping sets) |
| Tags | Mobile. Simple UWB responders. Not on the mesh. |
| Anchors | **Power may toggle** (on/off/return); **location may change (occasional relocation, slow)** |
| Coordinator | **No** single coordinator can manage all nodes |
| UWB library | mf-DW1000. Its **broadcast** RANGE frame holds at most **4 devices** (LEN_DATA=90) → many devices require **sequential polling** |

## 4. Confirmed Decisions

| # | Decision | Note |
|---|---|---|
| D1 | Anchor is the **polling master (initiator)**; tag is a **simple responder** | Only the polled tag transmits → no collision inside a slot. Mobility/handoff handled for free. |
| D2 | The thing being coordinated is the **anchor interference graph**, **not** a shared tag list | Each anchor polls its own current tags during its own turn. |
| D3 | Control plane = **ESP-WIFI-MESH** (multi-hop, self-organizing, rootless) | Separate radio from UWB. |
| D4 | Distributed agreement = **multi-agent DCOP**, algorithm **MGM (confirmed)** | Assigns slots/channels. |
| D5 | Color = **slot × channel** (both dimensions used) | More reuse, less contention. |
| D6 | **Collision-free is the primary mode**; contention (CSMA) is only a safety net during the relocation transient | Feasible because relocation is slow. |
| D7 | A backend **root is not required initially**; added later for aggregation | The esp_mesh internal root is auto-elected. |
| D8 | Priority (the original request) is absorbed as **demand weighting** + an **intra-anchor scheduler** | Far/weak tags get lower weight. |

## 5. Layered Architecture

| Layer | Role | Medium |
|---|---|---|
| L1 Connectivity | Multi-hop messaging, self-organizing/self-healing, auto-elected root | ESP-WIFI-MESH |
| L2 Sensing | Learn the anchor interference graph (who collides with whom). Low rate + on relocation events. | UWB overhearing + mesh gossip |
| **L3 Coordination (CORE)** | Distributed multi-agent color (slot×channel) assignment via MGM, lease-based | mesh |
| L4 Measurement | Each anchor polls its tags (TWR) during its owned slots on its channel | UWB |
| (L5 Aggregation) | *Later*: promote one node to a gateway → forward results | mesh → backend |

Tags sit **outside L1–L3** entirely.

## 6. Measurement Plane (L4)

- Anchor = initiator, tag = responder. **Only the polled tag transmits** → zero collision within a color.
- An anchor sequentially polls **only its own current tag set** during its owned slots.
- Because many tags exceed the broadcast 4-device limit, **single-device sequential polling** is used.
- Required mf-DW1000 changes (guarded, backward-compatible): bump `MAX_DEVICES`, expose a single-poll entry point, add a `_scheduledMode` flag, fix single-poll completion (POLL_ACK → single RANGE).

## 7. Control Plane (L1) — ESP-WIFI-MESH

- Self-organizing multi-hop tree, **auto-elected, self-healing root** (no dependence on a specific node; tolerates anchor power churn).
- Coordination traffic is **low rate** (a few packets per superframe): interference neighbors, demand, slot-phase, lease/slot state.
- Load mitigation: **pin cores** (WiFi on core 0, ranging loop on core 1), send control messages in the **slot guard interval**, **only anchors run WiFi** (zero load on tags).
- Caveat: in esp_mesh, node-to-node traffic routes via the root → frequent local gossip can bottleneck. If needed, use a **hybrid (ESP-NOW for local gossip + esp_mesh for structure/aggregation)** (decided in the scale-out phase).
- Aggregation promotion: later, attach one node to a router/backend → it becomes the gateway root; keep only an uplink hook in code for now.

## 8. Multi-Agent Coordination (L3, CORE) — summary

Detailed in [`DESIGN_distributed_tdma_mgm.md`](./DESIGN_distributed_tdma_mgm.md). In brief:

- **DCOP**: each anchor is an agent owning a slot variable; constraint = interfering neighbors must differ; objective = maximize reuse + allocate slots proportional to demand.
- **MGM**: per round, neighbors exchange current value and gain; only the strict local-max-gain agent changes → monotone, non-oscillating convergence.
- **Channel** is assigned per tag-sharing cluster (slow outer loop); **slot** is the per-agent MGM variable (fast inner loop).
- **Lease** handles power churn: an unrenewed color is reclaimed by neighbors.
- **Priority** enters as demand weighting (inter-anchor) plus an intra-anchor tag scheduler (L4).

## 9. Time Sync (slot phase)

- Only **ms-level slot-boundary alignment** is needed. TWR accuracy needs no clock sync.
- **Local gossip averaging** (rootless, churn/mobility tolerant). No global tight sync → avoids multi-hop error accumulation. Multi-hop jitter is absorbed by **generous guard times**.

## 10. Aggregation (L5, later)

- Now: keep the `logRange` CSV format (external EKF contract); reserve only an interface for pushing results onto the mesh.
- Later: anchor → gateway root → backend/EKF.

## 11. Roadmap (CORE-centric)

**[Foundation] (prerequisite for CORE — compressed but not skippable)**
- F-a: anchor-initiated UWB polling + simple tag responder (validate the pipeline with a few nodes)
- F-b: ESP-WIFI-MESH self-organizing (rootless) + **power/update-rate benchmark** + low-rate L2 sensing
- F-c: dynamic **lease-based distributed coloring** (DRAND-style) + contention safety net for the relocation transient

**[CORE] Priority-integrated distributed multi-agent slot×channel assignment (MGM)** ← main body (§8)

**[After]**
- Scale-out: multi-hop scaling / ESP-NOW hybrid if local gossip bottlenecks
- Aggregation: gateway promotion + result aggregation (L5)

## 12. Variant / Firmware Mapping

- `anchor_dw1000_*_meshagent`: UWB polling + L1–L3 (MGM agent). Macros `FEATURE_MESH` + `FEATURE_AGENT`. (Power profile conflicts with low-power/OLED variants — beware.)
- Tag: reuse existing simple-responder variant; only `MAX_DEVICES` is affected.
- No coordinator/root-specific variant (auto-elected). Gateway is an aggregation-phase build flag.
- Update `VARIANTS.md` when adding variants (project rule).

## 13. Risks / Assumptions

- **Relocation transient collisions**: short, absorbed by the contention safety net.
- **Power vs mesh**: always-on WiFi is power-hungry → power-save/duty-cycle, quantify in F-b benchmark.
- **mesh root bottleneck**: local ESP-NOW hybrid (scale-out phase).
- **Convergence vs change rate**: holds under the slow-relocation assumption; if change speeds up, the guarantee weakens and contention share grows.
- Assumes some anchors keep the mesh alive; node locations are not fixed (occasional moves).

## 14. Tunable Parameters / Open Items

- Slot count/length, UWB channel set (1/2/3/5), superframe period, guard time.
- Lease length, MGM round period, demand weight function `w(t)`, quality-streak thresholds.
- L2 sensing period / event triggers, gossip period.
- Power budget (confirmed by the F-b benchmark).
