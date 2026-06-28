# CORE 1st-Scope — Deep Design (single-channel, single-slot MGM + lease + intra-anchor scheduler)

Implementation-ready design for the first CORE pass. Builds on
[`DESIGN_P3_core_mgm.md`](./DESIGN_P3_core_mgm.md); terminology in
[`ARCHITECTURE_mesh_tdma.md`](./ARCHITECTURE_mesh_tdma.md) §1.

> Written in English per request. Design only — no code.

---

## A. Scope and resulting simplifications

- **In scope**: single channel + single-slot MGM (pure coloring) → leases → intra-anchor tag scheduler.
- **Deferred (2nd pass)**: multi-slot demand split (#3), channel outer loop (#5).
- **Confirmed decisions**: interference sensing uses **both shared-tag (mesh) AND UWB overhearing from
  the start**; initial parameter values accepted (§K).
- **Simplifications**:
  - Single channel → the N1/N2 edge distinction disappears → `C(i)` = "every anchor that interferes with i".
  - Single slot → **L3 is pure distributed graph coloring** (one slot per anchor). **All priority lives
    in L4** (intra-anchor tag scheduler) → the original request lands here.
  - Even with one slot, an anchor runs **several sequential TWR exchanges inside its slot** → throughput
    without multi-slot.

## B. Time structure (superframe / slot)

```
superframe = K_s slots (repeating)
slot       = [ guard | work-window | guard ]
            owner runs back-to-back TWR exchanges inside work-window (filled by L4)
            non-owner anchors RX-overhear during this slot (see D2)
```
- Initial estimates (to be measured/locked in F-a): `K_s=16`, `slotLen≈45ms`
  (guard 5 + work 35 + guard 5), exchange ≈ 5–8 ms → ~4–6 TWR per slot, superframe ≈ 720 ms.
- One slot per anchor → with 10 tags, per-tag update ≈ every 2 superframes ≈ 1.4 s (acceptable for PoC;
  improved by #3).

## C. Agent state (data structures)

```
AnchorAgent:
  id            : uint16            # own short address
  slot          : 0..K_s-1 | NONE
  state         : SEEKING | HELD
  leaseExpiry   : ms
  round         : uint16
  neighbors[]   : NeighborEntry     # interference neighbors (L2 output)
  audible[]     : AudibleEntry       # UWB-overheard anchors (L2 input)
  tags[]        : TagEntry           # L4

NeighborEntry : id, slot, leaseExpiry, lastHeard(ms), src{sharedTag|audible|both}
AudibleEntry  : anchorId, lastHeard(ms), rxp
TagEntry (L4) : shortAddr, score, lastPolled(ms), badStreak, agingRate, lastRange, lastRxp
```

## D. Interference sensing (L2) — two fused signals

The interference neighbor set is the **union** of two detectors (conservative; closes the
hidden-interferer gap):

`C(i) = { j : sharedTag(i,j) OR audible(i,j) OR audible(j,i) }`

### D1. Shared-tag (mesh)
- Each anchor periodically publishes its `tags[]` (short addresses) via the `TAGLIST` mesh message.
- Edge `sharedTag(i,j)` if the tag sets overlap. No extra UWB airtime.
- Limitation alone: misses anchors that interfere but currently list no common tag → fixed by D2.

### D2. UWB overhearing (radio)
- Outside its own slot, an anchor puts the DW1000 in **promiscuous RX** (frame filtering off) and records
  every overheard frame's **source short address + RX power** into `audible[]`.
- `audible(i,j)` is true if `rxp(j heard at i) > AUDIBLE_THRESH`.
- Audibility can be asymmetric; it is **symmetrized over mesh**: each anchor announces what it hears via
  the `AUDIBLE` message, so both endpoints add the edge.
- Cost: promiscuous RX in idle slots draws power. Acceptable for powered anchors; may be **duty-cycled**
  (overhear continuously at low rate, intensify for a few superframes after a `NBR_UPDATE(moved)`).
- Library implication: adds a **promiscuous-RX mode** that coexists with the library's TWR mode by
  switching at slot boundaries (see §J).

### D3. Edge aging
- Every neighbor/audible entry carries `lastHeard`; entries older than `leaseLen` are dropped
  (handles relocation/churn naturally).

## E. MGM round state machine (single-slot)

- Domain `{0..K_s-1}`, pick one slot. `cost_i = #{ j ∈ C(i) : slot_j == slot_i }`.
- Round on the control plane, period `T_round ≈ 500 ms`:
  1. **VALUE** send → collect neighbor VALUEs within a `T_collect` window.
  2. `slot* = argmin_s #{ j ∈ C(i) : slot_j == s }` (least-occupied slot; ties → lowest index).
     `Δ = cost_i(slot) − cost_i(slot*)`.
  3. **GAIN** send → collect neighbor GAINs.
  4. **DECIDE**: if `Δ > 0` and `(Δ, id)` strictly exceeds every neighbor's `(Δ_j, j)` → `slot = slot*`.
- **Async mesh handling**: rounds loosely aligned by `round` number; after `T_collect` timeout, proceed
  with whatever arrived (loss recovers next round). Late messages roll into the next round.
- Property: at most one neighbor moves per round → global conflict **monotonically non-increasing**
  (no oscillation, converges on a static graph).

## F. Lease mechanics

- `leaseLen ≈ 5 × superframe ≈ 3.6 s` (long, since change is slow). `leaseExpiry` piggybacks on `VALUE`,
  renewed each round.
- Neighbor expiry: `now − lastHeard > leaseLen` → drop entry → its slot becomes reusable.
- Self power loss → no renewal → neighbors reclaim automatically.

## G. Cold start / join

1. Boot → `state = SEEKING`, `slot = NONE`.
2. Listen one full round, collecting neighbor VALUEs (learn occupied slots).
3. Pick the least-conflict slot → publish VALUE → `HELD`.
4. Simultaneous joins resolve over the next rounds via MGM convergence.

## H. Edge cases

| Situation | Handling |
|---|---|
| No conflict-free slot (degree ≥ K_s) | pick min-conflict slot, flag residual; remedy = raise K_s or add channel (2nd pass) |
| Message loss | recovered next round (MGM tolerates loss) |
| Network partition | local convergence per partition |
| Relocation | `NBR_UPDATE(moved)` → re-sense (D1+D2) → local re-converge; transient covered by contention safety net (F-c) |

## I. L4 intra-anchor scheduler (the original request, concrete)

```
during my slot (repeat across the work-window):
  while time_left_in_window >= EXCHANGE_BUDGET:
    for t in tags: t.score += (now - t.lastPolled) * agingRate(t)
    t* = argmax score
    ok, range, rxp = twr_poll(t*)              # single-device DS-TWR
    if ok:
      logRange(idOf(t*), range, rxp)            # CSV format unchanged
      update badStreak(t*) from range/rxp
      agingRate(t*) = lowered if far/weak persists, restored on recovery
    t*.score = 0; t*.lastPolled = now
```
- `badStreak`: incremented while `range > FAR` or `rxp < WEAK`, reset to 0 on a good sample → far/weak
  tags polled less often (the original intent). Recovery resets to prevent starvation.

## J. Library touch points (mf-DW1000, guarded, backward-compatible)

| # | Change | Purpose |
|---|---|---|
| 1 | `MAX_DEVICES 4 → 12` | track up to ~10 tags |
| 2 | public single-poll entry | drive scheduled per-tag polling |
| 3 | `_scheduledMode` flag (default off) | disable auto-broadcast; preserve existing variants |
| 4 | single-poll completion fix (POLL_ACK → single RANGE) | current "last index" logic is broken |
| 5 | **promiscuous-RX mode** (filtering off, raw RX, parse MAC source) | UWB overhearing in idle slots (new for D2) |

## K. Wire formats (mesh, neighbor broadcast)

| msg | bytes |
|---|---|
| `VALUE` | type(1) round(2) agentId(2) slot(1) leaseExpiry(4) = **10 B** |
| `GAIN` | type(1) round(2) agentId(2) gain(2) propSlot(1) = **8 B** |
| `TAGLIST` | type(1) agentId(2) count(1) shortAddr[count]×2 → ≤ **24 B** (10 tags) |
| `AUDIBLE` | type(1) agentId(2) count(1) heardId[count]×2 → ≤ **24 B** |
| `NBR_UPDATE` | type(1) agentId(2) event(1) = **4 B** |

All low rate (a few packets per superframe) → small mesh load.

## L. Module decomposition (files; no code yet)

- `src/common/superframe.h` — slot/superframe timing (pure, host-testable; evolves the earlier `tdma_slot.h`)
- `src/common/peer_scheduler.h` — L4 tag aging scheduler (pure, host-testable)
- `src/common/mgm_agent.h` — L3 single-slot MGM logic (mesh/UWB-agnostic, pure, host-testable)
- `src/common/interference.h` — fused interference graph (shared-tag ∪ audible) + lease/aging
- `src/{anchor_dw1000_*_meshagent}/main.cpp` — wires ESP-MESH + UWB (TWR + promiscuous overhear) + agent
- mf-DW1000 hooks (§J)

## M. Parameters (initial, locked for F-a start)

`K_s=16`, `slotLen≈45ms`, `guard=5ms`, `EXCHANGE_BUDGET≈8ms`, `T_round≈500ms`, `T_collect≈200ms`,
`leaseLen≈3.6s`, `AUDIBLE_THRESH` (dBm, to calibrate), `agingRate_base`, `FAR`, `WEAK`, `badStreak`
threshold, `α`.

## N. Build order within this scope

1. `superframe.h` + `peer_scheduler.h` + `mgm_agent.h` + `interference.h` as **pure host-testable modules**.
2. mf-DW1000 hooks (§J), incl. promiscuous-RX.
3. `anchor_dw1000_*_meshagent` variant: wire UWB TWR + overhearing + ESP-MESH + agent.
4. Bench: collision-free operation, per-tag priority, overhearing-detected edges, WiFi/power/update-rate.
