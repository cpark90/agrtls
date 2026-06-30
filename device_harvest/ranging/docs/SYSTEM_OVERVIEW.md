# System overview

> The big picture: entities, roles, the anchor↔tag relationship graphs, and how ranging is scheduled.
> Terms: [GLOSSARY](GLOSSARY.md). Index: [README](README.md).

Top-level explanation of the whole system. Depth is in
[ARCHITECTURE_synchronous_tdma.md](ARCHITECTURE_synchronous_tdma.md) (authoritative); the evolution is in
[DESIGN_FLOW_distributed_tdma.md](DESIGN_FLOW_distributed_tdma.md); variants in [VARIANTS.md](VARIANTS.md).
Diagrams are given as Mermaid (renders on GitHub) **and** ASCII (visible anywhere).

---

## 1. Purpose

Measure distances between many **anchors** (fixed-ish infrastructure) and many **tags** (mobile), so a tag can be **localized** (trilaterated). The hard requirement: for each tag, its ranges from the several anchors must be **clustered in time** (a contemporaneous set) to form one position fix. Far or weak tags are measured **less often** (not dropped). Tags and anchors may move (occasionally) and power may toggle.

## 2. Entities and roles

| Entity | Mobility | Role (synchronous TDMA) | Notes |
|---|---|---|---|
| **Anchor** | fixed-ish, relocatable | **initiator** — polls tags; runs the ESP-NOW control mesh | judges far/weak from RXP |
| **Tag** | mobile | **responder** — replies to polls, **collects its own ranges** | may not hear other tags |

Two radios / two planes:

```
  control plane      :  Anchor  <-- ESP-NOW mesh -->  Anchor   (anchor-to-anchor coordination)
  measurement plane  :  Anchor  --- UWB poll (TWR) -->  Tag     (Tag responds, collects ranges)
```

UWB (measurement) and WiFi/ESP-NOW (coordination) are **separate radios** → no mutual interference. Tags are not on the mesh; the anchors mediate everything (tags can't always hear each other).

## 3. Anchor–Tag relationship graph (who can range whom)

The fundamental structure is a **bipartite graph**: an edge `A—T` means anchor `A` has an *effective* link to tag `T` (RXP/range good enough to range it, `q ≥ θ_link`). Example deployment:

```mermaid
graph LR
  subgraph Anchors
    A0; A1; A2; A3; A4; A5
  end
  subgraph Tags
    T0; T1; T2
  end
  A0 --- T0
  A1 --- T0
  A2 --- T0
  A2 --- T1
  A3 --- T1
  A4 --- T1
  A4 --- T2
  A5 --- T2
```

ASCII (effective links):

```
  tag  ranged by (effective anchors)
  ---  ------------------------------
  T0   A0, A1, A2
  T1   A2, A3, A4
  T2   A4, A5

  anchor  ranges tags
  ------  -----------
  A0      T0
  A1      T0
  A2      T0, T1      <- shared (border) anchor
  A3      T1
  A4      T1, T2      <- shared (border) anchor
  A5      T2
```

**Shared / border anchors** are anchors with edges to multiple tags — here **A2** ranges {T0,T1} and **A4** ranges {T1,T2}. A shared anchor cannot serve two tags at the same instant, so it forces those tags into different frames.

A tag's anchor set is its "cluster"; clusters **overlap** at shared anchors/tags. There is **no manual cluster definition** — this graph is discovered at runtime: each anchor publishes which tags it hears (+RXP) over the mesh, and the union is the bipartite graph.

## 4. Tag conflict graph → frames (spatial reuse)

Project the bipartite graph onto tags: **two tags conflict (need different frames) iff they share an *effective* anchor.**

```mermaid
graph LR
  T0 --- T1
  T1 --- T2
  %% T0 and T2 share no anchor -> no edge -> can reuse the same frame
```

ASCII:

```
  T0 ---- T1 ---- T2     edge = share an effective anchor
        (A2)   (A4)

  T0 & T1 : share A2   -> conflict (different frames)
  T1 & T2 : share A4   -> conflict (different frames)
  T0 & T2 : share none -> no conflict -> may reuse one frame
```

Color = frame. Color the tags so conflicting tags differ; **non-conflicting tags reuse a frame** (measured concurrently by disjoint anchor sets — spatial reuse):

```
  frame 0 :  T0  {A0,A1,A2}   +   T2  {A4,A5}     (disjoint anchors -> concurrent)
  frame 1 :  T1  {A2,A3,A4}
```

So only **2 frames** are needed even though there are 3 tags. `A2` ranges T0 in frame 0 and T1 in frame 1 (never double-booked); `A4` ranges T2 in frame 0 and T1 in frame 1.

## 5. Two-level TDMA timing

```
  Superframe = [ Frame 0 ][ Frame 1 ]  (repeats)

  Frame 0 :  A0->T0  A1->T0  A2->T0   |   A4->T2  A5->T2
              region 1 (tag T0)            region 2 (tag T2)   -> concurrent (disjoint anchors)
  Frame 1 :  A2->T1  A3->T1  A4->T1

  within a frame, the anchors poll in a fair slot order (no priority)
```

**Frame level** (which tag): tag→frame coloring, weighted so **far/weak tags get fewer frames**.

**Slot level** (within a frame): the participating anchors poll the frame's tag in a **fair** order (distributed coloring) so their polls don't collide.

Both are computed by the **anchor mesh**, and time is **gossip-synced locally** (only within a conflict neighborhood; far reuse regions need not align).

## 6. Measurement flow (one frame)

```mermaid
sequenceDiagram
  participant A0
  participant A1
  participant A2
  participant T0
  Note over A0,A2: Frame 0, tag = T0 (all known from the shared schedule)
  A0->>T0: POLL (slot 0)
  T0-->>A0: response  (A0/T0 compute range)
  A1->>T0: POLL (slot 1)
  T0-->>A1: response
  A2->>T0: POLL (slot 2)
  T0-->>A2: response
  Note over T0: T0 now has ranges to A0,A1,A2 — clustered → localization
```

A tag's ranges from all its (effective) anchors land inside its one frame → **temporally clustered**.

## 7. far/weak handling (three places)

| Where | What | Metric |
|---|---|---|
| **Anchor selection** (in a frame) | only anchors with a good link range the tag | `q(a,T) ≥ θ_link` |
| **Frame priority** (between conflicting tags) | far/weak tag gets fewer frames | `W̄_S = mean` of `q` over **shared effective anchors** (count-normalized, not a sum) |
| **Candidate** (tag with no eligible anchor) | not dropped; probed at min rate `K_probe`, re-enters if a link improves | — |

`q(a,T)` (link quality) is a **pluggable** function (first-cut `= RXP`), isolated so it can later use range / NLOS / geometry without touching the scheduler.

## 8. Coordination (all on the anchor mesh)

Anchors share a **tag registry** (`(tagId, rxp, range)` per anchor) over ESP-NOW, so the bipartite graph, weights, and the deterministic frame schedule are computed **identically** by every anchor.

**Hidden tags** (tags that can't hear each other but share an anchor) are serialized automatically: one tag per frame means only that tag transmits, so there is no collision even when they are mutually unheard.

**Emergent clusters**: coordination is local (a tag only needs a frame distinct from tags it conflicts with), shared anchors bridge regions, and the frames needed equal the local tag density, not the global tag count.

Mobility / power churn → registry refresh → graph + coloring re-converge locally.

## 9. Variant families (firmware)

| Family | Variants | Roles (tag / anchor) | Summary |
|---|---|---|---|
| **Standard** | `tag/anchor_dw1000_accuracy` | initiator / responder | native mf-DW1000 broadcast ranging, ≤4 anchors per tag, no mesh |
| **synchronous TDMA** | `anchor_dw1000_synchronous` + `tag_dw1000_responder` | responder / initiator (scheduled) | the frame system above (§3–§6): one shared deterministic schedule, time-clustered ranges |
| **distributed TDMA** | `anchor_dw1000_accuracy_meshagent` (+ the shared responder) | responder / initiator (MGM-negotiated) | earlier anchor-centric model: each anchor schedules its own tags, MGM slot×channel coordination |

See [`VARIANTS.md`](./VARIANTS.md). In both scheduled families the roles are **inverted** relative to Standard (anchor initiates, tag responds).

## 10. Status

- **Standard**: builds and does basic broadcast ranging.
- **Foundation** (shared by both scheduled families): anchor-initiated single-poll + ESP-NOW mesh coordination + epoch sync is validated on hardware (2 anchors).
- **synchronous TDMA**: the frame layer (§3–§6) is designed (this doc + `ARCHITECTURE_synchronous_tdma.md`); implementation pending (roadmap W-1…W-4 there).
- **distributed TDMA**: the MGM coordination is designed (`ARCHITECTURE_distributed_tdma.md` + `DESIGN_distributed_tdma_*`); the `*_meshagent` variant is its in-progress implementation.
