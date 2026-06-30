# Architecture — window-TDMA (authoritative)

> The **current/authoritative** ranging model: all anchors range the *same* tag per window so a tag's
> ranges are time-clustered for localization. Terms: [GLOSSARY](GLOSSARY.md). Index: [README](README.md).

Consolidated design after the requirement was clarified to **localization-grade ranging**: for a given
tag, all of its anchor ranges must be **temporally clustered** so they can be used together in a
position fix. This supersedes the earlier anchor-centric model in
[ARCHITECTURE_mesh_tdma.md](ARCHITECTURE_mesh_tdma.md) (kept as history).

---

## 1. Confirmed requirements

| # | Requirement |
|---|---|
| R1 | **Anchor is the initiator** (anchors poll; tags respond). |
| R2 | One measurement **window = exactly one tag**; in that window all anchors range that same tag. |
| R3 | A tag's ranges (from all anchors) must be **clustered in time** → used for localization. |
| R4 | The **range is obtained at the tag side** (tag ends up with its ranges to all anchors). |
| R5 | **Anchor↔anchor slot coordination** (within a window) is fair — **no** far/weak priority. |
| R6 | **far/weak tags are deprioritized** — measured in fewer windows. far/weak is judged **by the anchors** (from RXP/range). |
| R7 | Tags may be **out of communication range of each other** yet **share anchors** (hidden terminal). So tags cannot self-coordinate; the **anchors mediate**. |
| R8 | Anchors are numerous; tags are numerous. Both move (occasional relocation), power may toggle. |

## 2. Key consequence of R7 (hidden tags)

Two tags that cannot hear each other but share an anchor will collide at that anchor if both transmit. They cannot coordinate directly. The common point is the **anchors**, and the anchor is the initiator (R1), so **the anchor mesh owns the whole schedule**:

- All anchors agree that **window k is for tag `T_k`** → only `T_k` is polled/active in window k → no other tag transmits → **no hidden-tag collision**, and `T_k`'s ranges are clustered (R2, R3).
- Tags are pure responders; they do **not** self-schedule.

## 3. Structure — window-based two-level TDMA

```
Superframe = a sequence of WINDOWS (one tag each), repeating.
   schedule:  window 0 -> T_a,  window 1 -> T_b,  window 2 -> T_a, ...   (far/weak tags appear less)

Window(T_x) = a sequence of ANCHOR-SLOTS:
   [anchor 0 polls T_x][anchor 1 polls T_x]...   (fair order; anchors don't collide)

Result: every anchor ranges T_x inside Window(T_x) -> T_x's ranges are clustered.
```

Two coordination jobs, **both on the anchor mesh**:

| Job | Mechanism | Priority |
|---|---|---|
| **Window → tag schedule** | shared tag registry + weights → deterministic schedule | **far/weak ↓** (R6) |
| **Anchor-slot order** (within a window) | distributed coloring (MGM) | fair (R5) |

## 4. Window assignment: distributed tag→window coloring (+ far/weak weighting)

The window→tag assignment is a **distributed graph coloring of tags into windows**, computed by the anchor mesh from a shared tag registry. This subsumes the multi-cluster question and handles overlaps/spatial-reuse automatically.

### 4.1 Conflict graph and coloring
- Node = tag. **Edge (conflict) iff two tags share an _effective_ anchor** — an anchor that *actually ranges both* (each link passes `θ_link`, §4.3a), so it would have to serve both in one window. An anchor that merely *hears* both but ranges only one (the other link far/weak) is **not** a conflict point. Judging overlap by **effective anchors, not all hearing anchors**, avoids over-counting conflicts → more spatial reuse.
- Color = window. **Conflicting tags get different windows; non-conflicting tags reuse the same window** → spatial reuse (disjoint anchor regions measure concurrently).
- Computed by **distributed coloring (MGM)** over the anchor mesh (same machinery as the anchor-slot coloring, applied to tags), using the shared registry (which anchor hears which tags).
- Per anchor this means **exactly one tag per window** (its tags all have distinct colors) → never double-booked. A border tag has one color → all its anchors (across regions) measure it in that one window → ranges clustered (R3).

### 4.2 Clusters are emergent, not configured
- A "cluster" = anchors that (transitively) share tags — **determined by physical placement, discovered at runtime**: each anchor publishes the tags it hears (`TAGINFO`); "share a tag" = a coordination edge. No manual cluster assignment, no cluster IDs.
- Overlapping **anchors** are shared nodes that **bridge** regions of the conflict graph; overlapping **tags** simply get a single color. Coordination is **local** (a tag's color need only differ from tags it conflicts with) → windows needed = local tag density (max tags sharing one anchor), not the global tag count. Mobility/power churn → registry refresh → coloring re-converges locally.

### 4.3 far/weak via link quality: anchor selection + window priority
Everything keys off a per **anchor–tag link quality** `q(a, T)` (higher = nearer/stronger), which is **pluggable** (first-cut `q = rxp`; later add range/NLOS/geometry) and isolated from the scheduler.

**(a) Anchor selection within a window** — *which anchors in the cluster range this tag*: in tag T's window, anchor `a` ranges T only if `q(a,T) ≥ θ_link` (a good link). Anchors whose link to T is far/weak **skip it** → only good ranges are collected (accurate, useful for localization) and no airtime is wasted on unreliable links. This is **orthogonal to the fair anchor-slot order** (§3, R5): slots are fair; *selection* is by link quality.

**(b) Window priority between conflicting tags** — *which tag gets a contested window*: compare using **only their shared _effective_ anchors** (anchors that range *both* — same as the conflict definition in §4.1; apples-to-apples, calibration-robust), with a **count-normalized** aggregate so the number of anchors does not bias it:
- `W̄_S(T) = mean_{s ∈ sharedEffectiveAnchors(T1,T2)} q(s, T)` — the **mean** shared-anchor quality, **not** a sum. A raw sum would unfairly favor tags/clusters with more anchors (e.g. 10 weak links could out-sum 3 strong ones); the mean reflects far/weak independent of anchor count.
- higher `W̄_S` → near/strong → more windows; lower → far/weak → fewer.
- the aggregate is **pluggable** (mean first-cut; later median / k-th-best for robustness). All shared anchors compute the same value (same registry+formula) → consistent decision.

```
// tag_quality.h  (pure, swappable)
inline float linkQuality(float rxp_dBm, float range_m);  // higher = nearer/stronger; first-cut: = rxp_dBm
```

### 4.4 Time
`window index k = (now - epoch) / windowLen`, epoch gossip-synced **locally** (within conflict neighborhoods). Reuse regions need not be globally aligned (they don't interfere). Transient registry disagreement self-heals as it converges; rate-limit registry changes to minimize churn.

### 4.5 Candidate-only windows (no permanent exclusion / anti-starvation)
Anchor selection (§4.3a) can leave a tag with **no eligible anchor**. That tag is not dropped — it is kept as a **candidate** so it can re-enter when it moves closer / a link improves:

- **Candidate trigger**: if **every anchor in the cluster is excluded** for a tag (no link passes `θ_link` — the tag is far/weak from all of them), that tag's window becomes **candidate-only**: not normally measured, but **kept in the registry as a candidate** (cluster formation still tracks it), never deleted.
- **Probe (anti-starvation)**: each candidate is still measured at a **minimum rate** — at least one window per `K_probe` superframes. The probe re-measures all of the tag's links.
- **Re-entry**: if a probe shows some link is now good (`q ≥ θ_link`), the tag returns to active. So exclusion is reversible and follows mobility.
- `θ_link`, `K_probe` are pluggable alongside `linkQuality` (§4.3) — simple first-cut, easily tuned.

## 5. Measurement flow (one window)

1. Window k opens (epoch-synced). All anchors know `T_k` from the shared schedule.
2. Each anchor, in **its anchor-slot** (fair order), polls `T_k` (TWR). Anchor-slots are offset so the anchors' polls/exchanges don't overlap.
3. `T_k` responds to each anchor; the tag obtains its range to each anchor (R4) — all within window k → **clustered** (R3).
4. Next window → `T_{k+1}` per the schedule.

(Anchor-initiated TWR; the tag computes/collects ranges from the exchanges, or anchors report the range to the tag. SS-TWR or DS-TWR per the detailed design.)

## 6. Roles / coordination domains

| Entity | Role | Coordinates | Over |
|---|---|---|---|
| Anchor | initiator + infrastructure | window schedule (far/weak) **and** anchor-slot order (fair) | anchor mesh (ESP-NOW) |
| Tag | responder; collects its ranges | nothing (mediated by anchors) | — |

There is **no tag mesh** (tags can't reliably talk — R7). All coordination is anchor-side.

## 7. Reuse from F-a / F-b

- **ESP-NOW mesh** (anchor↔anchor) — window-TDMA carries `TAGINFO` + `SYNC` (`mesh_msg.h` +
  `mesh_wire.h`). The MGM `VALUE/GAIN/TAGLIST/AUDIBLE` messages are meshagent-only (`mgm_msg.h`, in
  the meshagent variant folder); `mesh_link.h` is the shared transport.
- **epoch gossip sync** — now aligns **windows** across anchors (not just slots).
- **MGM / interference** — assigns the fair **anchor-slot** order within a window.
- **peer_scheduler far/weak logic** — repurposed to compute **per-tag weight → window allocation**.
- **mf-DW1000 single-poll hooks** — anchor polls one tag (`T_k`) per slot.

## 8. Changes vs the current (F-a/F-b) implementation

- **Now**: each anchor independently polls *its own* discovered tags → a tag's ranges are spread across the superframe (NOT clustered). ✗ for localization.
- **New**: all anchors poll the **same** `T_k` per window → that tag's ranges are clustered. ✓
- Add: shared tag registry + weights, deterministic window→tag schedule, window timing layer.
- Keep: anchor mesh, epoch sync, MGM anchor-slot, single-poll.

## 9. Implementation plan (incremental)

- **W-1**: pure modules (host-testable):
  - `tag_registry.h` — merge per-anchor `(tagId, rxp, range)` reports; expose each tag's **effective** anchors (links passing `θ_link`) and the **effective shared** anchor set of any tag pair.
  - `window_color.h` conflict = two tags share an **effective** anchor (§4.1).
  - `tag_quality.h` — pluggable `linkQuality(rxp, range)` (first-cut: `= rxp`); anchor selection by `θ_link`; `W̄_S` (mean, count-normalized) comparison of two tags over their shared anchors.
  - `window_color.h` — distributed tag→window coloring (conflict = share-anchor), weighted by `W_S` (far/weak → fewer windows); candidate-only windows + `K_probe` anti-starvation; `tagForWindow(k)`.
  - extend `superframe.h` with the window / anchor-slot two-level timing.
- **W-2**: mesh `TAGINFO` message (tagId + RXP) in `mesh_msg.h`; share/merge over ESP-NOW.
- **W-3**: meshagent rework — poll `T_k` (shared) instead of own tags; anchor-slot offset within window.
- **W-4**: HW validation — multiple anchors + multiple tags (some far): confirm (a) a tag's ranges are clustered, (b) far tags measured less, (c) hidden tags don't collide.

## 10. Open parameters

windowLen, anchor-slot length, superframe/schedule length `L`, link-quality `linkQuality` (RXP/range→quality), anchor-selection threshold `θ_link`, candidate probe rate `K_probe`, registry update/rate-limit period, epoch-sync period.

Implemented tunables (§11.5): `slotLen`/guard (`WindowFrameConfig`), `MAX_SLOTS_PER_WINDOW` (slot cap),
`WC_MAX_WINDOWS` (active-window cap), `TR_ENTRY_TTL_MS` (entry aging), responder `INACTIVITY_TIME`
(≥ superframe), `TQ_LINK_THRESH` / `TQ_HYSTERESIS` / `TQ_EMA_ALPHA` (link quality). `numWindows` and
`slotsPerWindow` are derived dynamically from the shared registry.

## 11. Responder layer & hardware validation (implemented)

§1–9 are the scheduler (anchor) design. HW validation (W-4) showed the **responder (physical tag)
layer** — the stock mf-DW1000 single-peer design — broke the no-permanent-exclusion invariant, so it
was hardened too. This section documents the implemented + HW-validated layers.

### 11.1 Responder layer (mf-DW1000, `_type==ANCHOR`)
Under role inversion the physical tag is the responder. For several anchors to range one tag
(clustering), the responder must hold and serve many initiators at once:

- **multi-device hold**: drop the "1 TAG" reset in `addNetworkDevices()` (+ a `MAX_DEVICES` bound) so
  the responder registers many anchors simultaneously.
- **per-device protocol state**: move `_expectedMsgId`/`_protocolFailed` onto `DW1000Device`, so
  interleaved POLL→RANGE exchanges from different anchors don't clobber each other.
- **admit-on-POLL (with target check)**: an unknown initiator whose POLL is **addressed to us**
  re-registers immediately — an aged-out anchor rejoins without a fresh BLINK (the §4.5 candidate
  principle, enforced at the radio layer). A POLL overheard for another tag does **not** register the
  sender (avoids table churn).
- **short-address dedup**: device identity = short address (prevents an admit-on-POLL entry and a
  BLINK entry duplicating one anchor).
- **per-model loops + destination check**: `DW1000Ranging` has a single-role loop per model —
  `loop()` (native), `loopInitiator()`/`loopResponder()` (window-TDMA), `loopMeshagent()` (mesh-TDMA).
  The TDMA loops add a destination check (`frameForMe`): a unicast frame overheard for *another* node
  (e.g. another anchor's `POLL_ACK`/`RANGE_REPORT`) is ignored, so an anchor never logs another
  anchor's measurement as its own.

### 11.2 Anchor polling / windows (`anchor_dw1000_window`)
- **dynamic window count**: the frame cycles `activeW = wc.numWindows()` active windows **+ one
  trailing probe window**, never empty windows. The probe window is always present so the per-anchor
  superframe length (`activeW+1`) is shared-deterministic → epoch sync stays aligned.
- **probe / bootstrap target** = a discovered tag this anchor is **not yet effective** for (covers
  bootstrap = never-measured, global candidates, weak-link reprobe). Without it an anchor could never
  start ranging an active tag it hadn't measured (it isn't in `tagForWindow` until effective → deadlock).
- **one poll per slot**: no greedy re-poll, so an exchange never spills past the slot edge. (Anchor-slot
  assignment and dynamic `slotsPerWindow`: §11.5.)
- **no fake -120 timeout report**: a single transient timeout must not overwrite a good link and flip
  the tag to candidate (which would desync coloring/epoch).

### 11.3 Validation (2 anchors + 2 tags)
- ✓ **clustering**: both anchors range the same tag (T0). No permanent exclusion — the weak tag (T1)
  is still measured via the probe window.
- ✓ minimal device-table churn; per-device state keeps concurrent exchanges stable.
- ✓ **link-quality stabilization**: a link near the threshold (RXP mean ≈ −85, spread −67…−102 from
  multipath) flipped eligibility every sample → coloring oscillated active↔candidate → the superframe
  length wobbled → epoch desynced. Fixed in `tag_quality.h` with **EMA smoothing + hysteresis**: the
  registry keeps a smoothed RXP and a hysteretic `eligible` state per link, so one noisy sample can't
  move the schedule. Result: coloring stable, anchors agree, epoch steady; a genuinely marginal link
  settles as candidate (excluded from clustering but still probed — correct for localization quality).
  Params: `TQ_LINK_THRESH` (−85), `TQ_HYSTERESIS` (±3 dB), `TQ_EMA_ALPHA` (0.3) — pluggable.

> Note: if an anchor can't even **discover** a marginal tag (weak BLINK↔RANGING_INIT round-trip), that
> is a physical-link limit (§11.6). Such a tag is ranged only by the anchors that reach it, degrading
> gracefully via candidate+probe.

### 11.4 Dev tooling
- `.claude/skills/uwb-bench` — node build/upload (`flash.sh`), non-TTY serial capture
  (`serial_capture.py`), per-device range tally (`tally.py`). Used because `pio device monitor` needs
  an interactive TTY and fails in a non-interactive shell.

### 11.5 Anchor-slot coloring · dynamic slots · aging · cross-anchor consistency (implemented)

Anchor-slot assignment within a window for the 5–10 anchor case, plus the consistency that keeps it
from diverging across anchors:

- **deterministic anchor-slot coloring**: an anchor's slot within a window = its **rank among the
  window tag's effective anchors** (`TagRegistry::anchorRank`, by ascending id). Anchors ranging the
  same tag get distinct slots → collision-free, computed deterministically from the shared registry
  (like the window coloring), so **no MGM negotiation is needed** (MGM is only needed if the
  shared-registry assumption is dropped).
- **dynamic `slotsPerWindow`** = max effective anchors over any tag (`maxEffectiveAnchorCount`,
  `WindowFrame::setSlotsPerWindow`, capped by `MAX_SLOTS_PER_WINDOW`) → exactly enough slots, no waste.
- **inactivity timeout**: the superframe (`numWindows·slotsPerWindow·slotLen`) can exceed 1 s as anchors
  grow, so the responder `INACTIVITY_TIME` was raised 1000→6000 ms (admit-on-POLL is the backstop);
  raise further if the superframe approaches it.
- **registry entry aging**: per-link `lastMs` + `TagRegistry::expire(now, TR_ENTRY_TTL_MS=5000)` drops
  a vanished (anchor,tag) link so a gone tag stops being scheduled / inflating slot counts
  (`nowMs==0` reports never expire — for host tests).
- **owner-authoritative eligibility (the consistency key)**: a link's eligibility is decided **once by
  the owner anchor** (from its own smoothed history), published in the TAGINFO `eligible` bit, and
  adopted verbatim by receivers (`reportPeer`). If each anchor recomputed EMA/hysteresis independently,
  a marginal link would be eligible on one and not another → divergent slots/coloring → epoch drift.
  Anchors publish their own links (smoothed RXP + decision) via `selfLinks`.
- **validation (2 anchors + 2 tags)**: both anchors agree on `slots` and `sched` (previously diverged
  1 vs 2). `anchorRank` / aging / `maxEffectiveAnchorCount` are host-tested.

### 11.6 Physical-link limits (outside firmware — placement / calibration)

Even with correct scheduler + responder logic, **a weak link can fail at discovery** (the
BLINK↔RANGING_INIT round-trip) — e.g. A1 never discovers a far T0. This is a physical limit, not a bug;
mitigations:

- **anchor placement / height**: keep each tag within LOS of ≥3 anchors (localization needs ≥3); avoid
  high-multipath spots.
- **antenna-delay calibration** (`CLAUDE.md` #1 suspect): never ship the default (16384/16385);
  calibrate per module. Uncalibrated delay skews range/RXP and can mis-judge marginal links near `θ_link`.
- **TX power / RF mode**: at close range, near-saturation (>−40 dBm) → lower TX; at long range, use
  ACCURACY mode (110 k / long preamble) for sensitivity.
- The system degrades gracefully (candidate+probe) for anchor-tag pairs that don't reach, so it keeps
  working with the anchors that do — but that tag's cluster shrinks and its position accuracy drops.
