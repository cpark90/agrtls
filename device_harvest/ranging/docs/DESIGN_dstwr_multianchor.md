# DS-TWR engine — multi-anchor reception

> Restructure the mf-DW1000 DS-TWR firmware engine so that "frames from several anchors are always
> interleaved on the air" is a first-class assumption, structurally preventing cross-exchange
> confusion. Terms: [GLOSSARY](GLOSSARY.md). Index: [README](README.md).

This is a **firmware/library-level** design for the shared ranging engine
(`lib/mf_DW1000`), independent of which TDMA model sits above it
([synchronous](ARCHITECTURE_synchronous_tdma.md) or
[distributed](ARCHITECTURE_distributed_tdma.md)). Design only — no code.

---

## 0. Motivation (what broke, and why)

Empirically, the native/stock engine is correct with **one anchor + one tag** (1330 ranging samples
captured with zero bad values, `round1−reply1` stable to ±100 device-time units), but starts emitting
**negative / huge ranges the moment a second anchor transmits** (~4–8% of samples). Diagnosis (per-sample
instrumentation of the four DS-TWR intervals + raw timestamps + signal quality) showed:

- the bad samples are **signal-independent** (RXP / first-path / noise identical to good samples) and
  **discrete** (jumps, not a continuous tail) — so not first-path wobble;
- the corrupted quantity is a **received-frame timestamp** (`pollRecv` mainly), which jumps in **both
  directions** (`round1−reply1` ranged from −21149 to ~+3.4e11 ≈ 0.3·2⁴⁰, the latter overflowing the
  int64 product), i.e. a **race**, not a bias.

The corruption appears only when another anchor is on the air → the engine mixes frames between
exchanges. The fix is not a patch on one race; it is to make the engine **assume interleaving**.

## 1. Why the current engine conflates exchanges

| # | Structural flaw | Consequence under multi-anchor |
|---|---|---|
| **R1** | The RX timestamp is read from the **live `RX_TIME` register**, **separately** from the frame data, **after** decode / lookup / `transmitPollAck` processing (tens–hundreds of µs later). | Permanent-receive mode lets a **later frame overwrite `RX_TIME`** in that gap → the data and the timestamp belong to **different frames**. |
| **R2** | Exchange state is **global mutable**: `_lastSentToShortAddress`, `_lastPollAckShortAddress`, `_replyDelayTimeUS`, `_expectedMsgId`, and the single `data[]` buffer. | One anchor's exchange **overwrites another's** state. |
| **R3** | **Permanent promiscuous receive** + slow loop processing. | Frames from *other* anchors' exchanges are processed/associated with the wrong peer. |

Common root: the engine implicitly assumes **one exchange in flight at a time**, but with N anchors,
N exchanges interleave.

## 2. Design principle (one line)

> **Capture each frame atomically at reception (data + timestamp + quality together); afterwards work
> only on that immutable copy, keyed by peer, with a pure computation. Never touch a hardware RX
> register again during processing.**

This single rule addresses R1, R2 and R3 at once.

## 3. The redesign

### 3.1 Atomic reception capture — an `RxFrame` descriptor (removes R1)
Immediately on reception, **before any processing**, read everything about the frame back-to-back into
one immutable value:

```cpp
struct RxFrame {
    uint8_t    data[LEN_DATA];
    uint8_t    len;
    DW1000Time ts;            // this frame's RX timestamp
    float      rxp, fp, noise;
};

// at the top of the loop's _receivedAck handler:
RxFrame f;
DW1000.getData(f.data, &f.len);
DW1000.getReceiveTimestamp(f.ts);              // µs after getData -> same frame guaranteed
f.rxp = DW1000.getReceivePower();
f.fp  = DW1000.getFirstPathPower();
DW1000.startReceive();                          // re-arm RX at once (don't drop the next frame)
// from here on, process f only; do not read RX_TIME / RX power registers again.
```

A UWB frame takes **>100 µs** to be received, whereas `getData`→`getReceiveTimestamp` are **µs-apart
back-to-back SPI reads**, so no incoming frame can update `RX_TIME` between them → data and timestamp
**always belong to the same frame**. The range-bias correction also reads the matching frame's power.

> **Optional hardening (deferred):** the DW1000 **RX double buffer** (`SYS_CFG` RXDBUF + HSRBP/ICRBP
> swap) lets the host read buffer set A while the radio receives into set B (lossless). It adds
> buffer-pointer management → against the "simplify" goal, so it is **not** part of the core change;
> the atomic read above is sufficient.

### 3.2 Per-peer state (removes R2)
`DW1000Device` (keyed by anchor short address) already holds the per-exchange timestamps. **Absorb the
remaining globals into the peer record and delete the globals:**

| Global (delete) | Becomes |
|---|---|
| `_expectedMsgId` | `peer.expectedMsg` (per-device version already exists) |
| `_replyDelayTimeUS` | `peer.replyDelayUS` |
| single `data[]` | the stack-local `RxFrame f` (never shared across exchanges) |

Two anchors' exchanges then **cannot touch the same variable** — confusion is impossible by
construction, not by timing luck.

### 3.3 Single pending-TX attribution (removes the POLL_ACK send-time crosstalk)
The DW1000 has **one TX engine** → at most one transmission in flight. Replace the address-keyed global
`_lastPollAckShortAddress` with a single TX descriptor that names the peer directly:

```cpp
struct { DW1000Device* peer; uint8_t type; bool busy; } _txPending;

// just before transmit():           _txPending = { peer, POLL_ACK, true };
// on TX-complete:    if (_txPending.busy) {
//                        DW1000.getTransmitTimestamp(ts);
//                        store ts on _txPending.peer according to _txPending.type;
//                        _txPending.busy = false;
//                    }
// rule: do NOT schedule a new TX while _txPending.busy (consume the previous completion first).
```

The "no re-schedule while busy" rule prevents a second anchor's POLL_ACK from stealing the attribution
before the first completes (the hardware allows only one TX anyway, so this matches reality).

### 3.4 Self-describing frames (detect mis-association)
Carry, in every frame, the **source address** (already present) **plus an exchange / sequence id**, so a
receiver can confirm "this RANGE belongs to the **same exchange** as this peer's POLL". On mismatch,
**skip that exchange** rather than combine timestamps from different exchanges. This also closes the
stale-timestamp class of bug (a RANGE pairing with a previous cycle's local timestamps).

### 3.5 Exchange isolation — one exchange is never disturbed by another
Each peer is an **independent DS-TWR state machine with its own timestamp set**, so N exchanges run
concurrently and share only the radio (time-multiplexed at the frame level by §3.1 / §3.3), **never
state**. The rule that guarantees it is **route-then-mutate**:

```
on RxFrame f:
    if !frameForMe(f):  drop                       # overheard / addressed to another node
    peer = peers.find(srcAddr(f))                   # 1) route to the source peer's context FIRST
    if peer == null:    (admit, or drop)
    peer.onFrame(f)                                 # 2) mutate ONLY that peer's context
```

Because a frame is dispatched to its **source peer's** context before any state changes, a frame from
peer B can **never advance, reset, or corrupt** peer A's in-flight exchange. The only resources shared
across peers are the single TX engine (serialized by §3.3's "one in flight" rule) and the RX register
(made per-frame-private by §3.1's atomic capture) — both already isolated. Isolation therefore **falls
out of §3.1–§3.3**; this section just states it as an explicit invariant to preserve.

### 3.6 Exchange lifecycle — detect a stalled exchange and flush it
A lost frame (signal not received) leaves an exchange **half-filled**: some of the six timestamps are
set, the rest never arrive. If that residue lingers, a later/stray frame can combine it into a garbage
range. Model each peer's exchange as an **explicit lifecycle with a per-peer deadline**:

```
IDLE ──(POLL seen/sent)──▶ ACTIVE(awaiting next frame, deadline = now + timeout(state))
ACTIVE ──(expected frame from THIS peer)──▶ advance ... ──▶ COMPLETE (emit range) ──▶ IDLE
ACTIVE ──(now > deadline: expected frame never came)──▶ FLUSH ──▶ IDLE
```

Per-peer fields: `active`, `expectedMsg`, `deadlineMs`. Entering a waiting state stamps
`deadlineMs = now + timeout(state)`, where `timeout ≈ expected reply delay + guard` (derived from the
protocol timing). A **lightweight periodic tick** (reuse the existing `timerTick`) scans peers:

```
for peer in peers:
    if peer.active && now > peer.deadlineMs:
        peer.flush()        # the expected frame was not received -> abandon this exchange
```

`flush()` is **one cheap call** that (a) clears `active` / resets `expectedMsg` to IDLE, and
(b) **invalidates the half-filled timestamps** so they can never be paired into a range. Because all
exchange state lives in a single per-peer struct (§3.2), there is **no scattered cleanup** — this is the
simplification payoff. Key properties:

- **Per-peer, not global**: one peer's stall flushes only that peer; the others keep ranging
  undisturbed (reinforces §3.5).
- **Free detection**: no extra timers/threads — it rides the periodic tick already in the loop.
- **Closes stale-pairing**: a half-finished exchange is always cleaned on stall, so an old cycle's
  timestamps cannot be resurrected by a late frame (complements §3.4).
- **Feeds back upward**: a flush = a missed frame; surface it as a link-quality / miss signal the
  scheduling layer can use (fewer slots / re-probe for a weak link).

## 4. Relationship to the scheduling layer

The engine and the TDMA layer are complementary:

- **TDMA** (the [synchronous](ARCHITECTURE_synchronous_tdma.md) / [distributed](ARCHITECTURE_distributed_tdma.md)
  models) *reduces* collisions ahead of time by giving anchors separate slots.
- **This engine redesign** *defends against* the interference that remains — overheard frames, imperfect
  slot guards, churn transients.

If the engine treats "frames are always interleaved" as the norm, the schedule can be loose without the
firmware ever emitting a corrupted range.

## 5. Simplification effect & migration order

**Simpler, not more complex** (per the project's "prefer the simplest form; remove branches" rule):
- the 3–4 scattered `getReceiveTimestamp(...)` calls collapse to **one read per loop**;
- **four globals are deleted**;
- `loop()` / `loopInitiator()` / `loopResponder()` / `loopMeshagent()` share one **capture → pure
  process** skeleton, removing duplicated logic;
- cleanup of a lost exchange is a **single per-peer `flush()`**, not scattered conditionals.

Apply incrementally, lowest risk first:

1. **§3.1 atomic capture** — closes the dominant race; most bad values disappear immediately.
2. **§3.2 / §3.3 global elimination** — makes cross-exchange confusion structurally impossible and
   simplifies the code; **§3.5 exchange isolation** falls out of this.
3. **§3.6 exchange lifecycle** — per-peer stall detection + `flush()` (handles lost frames / non-reception).
4. **§3.4 sequence id** — completeness (also closes stale-pairing).

Each step is independently shippable and independently verifiable on hardware (re-run with ≥2 anchors;
the bad-sample rate should drop to zero, and a powered-off anchor's exchange should flush cleanly
without disturbing the others).
