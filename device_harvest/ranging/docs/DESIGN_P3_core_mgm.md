# CORE (P3) Detailed Design — Multi-Agent Slot×Channel Coordination (MGM)

Detailed design of the coordination core. See [`ARCHITECTURE_mesh_tdma.md`](./ARCHITECTURE_mesh_tdma.md)
for the overall system and §1 there for shared terminology. Design only — no code.

> Note: written in English per explicit request, for clarity.

---

## 0. P3 spans two layers (the key split)

| Part | Where | Job | Origin |
|---|---|---|---|
| **L3 (inter-anchor)** | control plane (mesh) | MGM assigns slot×channel → **divides airtime among anchors** | multi-agent decision |
| **L4-sched (intra-anchor)** | anchor local | inside its own slots, **decide which tag to poll** via aging | the **original request** |

So "measure far/weak tags less often" lives in **L4-sched** (per-tag priority inside an anchor), while
"a busy/healthy anchor gets more slots" lives in **L3** (demand split between anchors). Both belong to P3.

## 1. Color domain and the channel constraint (per D5)

`color = (slot, channel)`. But a **simple tag receives on only one channel at a time**, so all anchors
that can range a given tag must be on the **same channel**. This yields two edge types in the
interference graph:

- **N1 (tag-sharing edge)** — anchors *i*, *j* may see a common tag → must use **same channel,
  different slot**.
- **N2 (interference-only edge)** — *i*, *j* hear each other but share no tag → only need **different
  color** (differ in slot *or* channel). They may reuse the **same slot** if on **different channels**.

**Decomposition (keeps it tractable):**
- **Channel = per tag-cluster.** A tag-cluster is a connected component under N1 edges; the whole cluster
  uses one channel (so shared tags stay single-channel). This is a slow **outer loop** (§6).
- **Slot = per-agent variable** assigned by MGM. This is the fast **inner loop** (§4), a slot-coloring on
  the same-channel subgraph.

**Honest refinement:** the price of channels is that a tag is not perfectly "dumb" — on handoff it must
**find its channel** (scan which channel it is being polled on; no schedule knowledge needed). In steady
state it stays on a fixed channel.

## 2. DCOP formalization

- Agent = anchor *i*. Variable = `s_i` (slot; channel is set by its cluster). Domain = `{0 .. K_s-1}`.
- **Conflict set** `C(i) = N1(i) ∪ { j ∈ N2(i) : channel(j) = channel(i) }`.
- **Hard cost** `cost_i = Σ_{j ∈ C(i)} 1[s_i = s_j]` (number of slot conflicts). Global target: 0.
- **Soft objective**: slot count proportional to demand (multi-slot extension, §3).

## 3. Demand / priority model (Tier B, the L3 side)

- Anchor demand: `d_i = Σ_{t ∈ T_i} w(t)`.
- Tag weight: `w(t) = max(w_min, 1 − α · badStreak(t))`.
  - `badStreak(t)` = consecutive samples where `range > FAR` **or** `RXP < WEAK`; reset to 0 on a good sample.
  - Far/weak tags → lower `w` → lower anchor demand.
- Slots requested by anchor *i*: `m_i = clamp( round( K_s · d_i / Σ_{j ∈ C(i)∪{i}} d_j ), 1, K_s )`.
  - Demand-proportional fair share among neighbors; a busy anchor gets more slots.

## 4. MGM in detail (confirmed algorithm)

**Three phases per round** (neighbor-scoped, synchronous rounds):
1. **VALUE**: broadcast my slot set `Sl_i` (size `m_i`) to neighbors.
2. **GAIN**: compute best alternative `Sl_i*` and gain `Δ_i = cost_i(Sl_i) − cost_i(Sl_i*)`; broadcast it.
3. **DECIDE**: set `Sl_i ← Sl_i*` **iff** `Δ_i > 0` **and** `Δ_i` strictly exceeds every neighbor's gain
   (break ties by larger `agent_id`).
   → at most one neighbor moves at a time → **global conflict is monotonically non-increasing**
   (no oscillation; convergence guaranteed on a static graph).

**Computing `Sl_i*` (multi-slot):** pick the `m_i` slots that least overlap conflicting neighbors' slots
(greedy: sort candidate slots by how many neighbors in `C(i)` occupy them, ascending).

**Domain exhaustion** (neighborhood degree / demand sum > `K_s`, so zero-conflict is impossible):
degrade to **minimize weighted conflict**; residual conflicts are yielded by **low-demand (low-priority)
pairs**. Pressure valves: the channel outer loop (§6) splits the subgraph, or increase `K_s` (longer
superframe, slower updates — a trade-off).

## 5. Lease state machine (power churn)

```
SEEKING ──(MGM finds a conflict-free Sl)──▶ HELD
HELD ──(renew each superframe/round)──▶ HELD
HELD ──(observe a neighbor's lease lapse)──▶ drop it from C(i); its slots become reusable
self power loss → no renewal → neighbors mark it EXPIRED → slots reclaimed
return → SEEKING (re-acquire after hearing neighbors' current Sl)
```

- Lease length: **long** (relocation is slow). Renewal piggybacks on the VALUE message.

## 6. Channel outer loop (per-cluster channel assignment)

- A tag-cluster = a connected component under N1 edges; assign it one channel.
- Goal: give **N2-adjacent clusters different channels** (so they can reuse the same slot). Channel set =
  usable UWB channels (e.g., 1/2/3/5).
- Slow agreement (a cluster representative greedily picks a channel given neighboring clusters' channels,
  via gossip). Channel changes are rare (only on relocation/congestion).
- Tags crossing a cluster boundary do the "find my channel" step (§1).

## 7. Intra-anchor tag scheduler (L4-sched — the original request, verbatim)

For each poll opportunity (one of the anchor's owned slots), choose which tag to poll:
- Per tag: `score += (now − lastPolled) · agingRate(t)` (older waits rank higher).
- `pick()` = the max-score tag → after polling, `score = 0` (the penalty = sent to the back).
- Persistent far/weak (`badStreak` rising) → lower that tag's `agingRate` → polled less often
  (the original intent).
- Recovery resets the streak (prevents starvation). This is the `peer_scheduler` logic envisioned early on.

In short: **L3 gives an anchor a slot budget; L4-sched spends that budget across tags by priority.**

## 8. Interfaces

**L2 → L3** (input to anchor *i*): `N1(i)`, `N2(i)` (each neighbor's current channel and lease), and
demand `d_i` (derived from L4's tag set + quality).
**L3 → L4** (output): `channel_i`, `Sl_i` (slot set), superframe params (`K_s`, `slotLen`, `guard`). L4
polls only during `Sl_i` on `channel_i`.

## 9. Message formats (mesh, neighbor-scoped)

| Message | Fields |
|---|---|
| `VALUE` | round, agentId, channel, slotSet (K_s-bit mask), demand `d_i`, leaseExpiry |
| `GAIN` | round, agentId, gain `Δ_i`, proposedSlotSet |
| `NBR_UPDATE` | agentId, event ∈ {moved, join, leave} (relocation/churn) |

- `slotSet` is a `K_s`-bit mask (≤ 4 bytes if `K_s ≤ 32`). Low rate → small mesh load.

## 10. Round timing / synchronization

- MGM rounds run **in the background on the control plane**, slowly (e.g., one round per few hundred ms to
  a few seconds). The UWB TDMA (L4) runs each superframe on the **current assignment**, independently —
  the two are decoupled.
- Loose round sync: carry the round number in `VALUE` and align with neighbors. Lost messages recover on
  the next round (MGM tolerates loss).

## 11. Dynamics

- **Demand change (tags moving)**: `d_i`, `m_i` update → MGM rebalances slot counts (slow).
- **Relocation**: an anchor sends `NBR_UPDATE(moved)` → L2 re-senses → `C(i)` changes → MGM re-converges
  locally; the transient is covered by the contention safety net (F-c).
- **Churn**: handled by the lease (§5).

## 12. Parameters

`K_s` (slot count), `slotLen`, `guard`, channel set, `agingRate` (L4), `α` / `w_min` / `FAR` / `WEAK` /
`badStreak` thresholds (demand/tag), lease length, MGM round period, gossip period.

## 13. Pseudocode (agent main, design level)

```
# L3 MGM (control plane, each round)
on_round(r):
    broadcast VALUE(r, id, ch, Sl, d, leaseExpiry)          # 1 VALUE
    collect neighbors' VALUE
    C = N1 ∪ {j in N2 : ch_j == ch}
    Sl_star = greedy_min_conflict_slots(C, m_i)
    delta   = cost(Sl, C) - cost(Sl_star, C)
    broadcast GAIN(r, id, delta, Sl_star)                   # 1 GAIN
    collect neighbors' GAIN
    if delta > 0 and (delta, id) > max(neighbor (delta_j, j)):
        Sl = Sl_star                                        # DECIDE
    renew_lease()

# L4 UWB (measurement plane, each poll opportunity in Sl on channel_i)
poll_opportunity():
    for t in T_i: t.score += (now - t.last) * agingRate(t)
    t_star = argmax_score(T_i)
    twr_poll(t_star)
    update badStreak / quality(t_star)
    t_star.score = 0; t_star.last = now
    recompute d_i from { w(t) }
```

## 14. Open items / risks

- `K_s` vs maximum local degree: high degree → exhaustion → more reliance on the channel outer loop.
  Need the measured degree distribution.
- Multi-slot MGM convergence speed — validate single-slot first, then extend.
- Tag "find my channel" latency on handoff — start with few channels (or a single channel) to minimize impact.
- L3 round period vs relocation frequency — comfortable under the slow-relocation assumption.

## 15. Suggested build order (within CORE)

1. **Single channel + single slot MGM** (pure coloring) → collision-free skeleton.
2. Add **leases** (churn).
3. Add **multi-slot (demand-weighted)** → L3 priority.
4. Add the **intra-anchor tag scheduler** (the original request).
5. Add the **channel outer loop** → slot reuse expansion.

Recommended 1st CORE scope: **1 → 2 → 4**; defer **3 and 5** to a 2nd pass.
