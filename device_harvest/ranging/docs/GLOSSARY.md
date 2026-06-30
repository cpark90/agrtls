# Glossary

Standard / formal terms used across the docs — UWB ranging, TDMA, and distributed-coordination
vocabulary that a reader is expected to already recognize. **Project-specific design terms** (the
two TDMA models, windows, the scheduling concepts, message and module names) are defined where they
are used, in their design documents — not here.

Index: [README.md](README.md).

---

## Roles

| Term | Meaning |
|---|---|
| **anchor** | A fixed station at a known position. Several anchors range a tag to localize it. |
| **tag** | A mobile node whose position is wanted. |
| **initiator** | The node that **starts** a ranging exchange (sends the first message, `POLL`). |
| **responder** | The node that **replies** to the initiator's poll. |

## UWB ranging

| Term | Meaning |
|---|---|
| **TWR** | Two-Way Ranging — distance from message round-trip time. |
| **DS-TWR** | Double-Sided TWR (`POLL → POLL_ACK → RANGE → RANGE_REPORT`); uses two round trips so the clock-offset error cancels. |
| **RXP** | Received signal power (dBm) of a frame; the primary link-quality indicator. |
| **LOS / NLOS** | Line-Of-Sight / Non-Line-Of-Sight. NLOS (obstruction, multipath) inflates the measured range and lowers RXP. |
| **antenna delay** | The fixed TX+RX signal delay of a UWB module, calibrated per device; its error maps directly to a range bias. |

## TDMA / timing

| Term | Meaning |
|---|---|
| **TDMA** | Time-Division Multiple Access — nodes transmit in separate time slots to avoid collisions. |
| **superframe** | The repeating top-level TDMA period; subdivided into slots. |
| **slot** | One time division of the superframe; assigned to a single transmitter. |
| **guard** | Idle margin at a slot's edges so one transmission cannot spill into the next slot. |
| **epoch** | The time origin of the schedule; nodes align their epochs (time sync) so their slots fall at the same absolute times. |

## Coordination / networking

| Term | Meaning |
|---|---|
| **ESP-NOW** | Espressif connectionless WiFi protocol for direct device-to-device frames; used as the control plane (separate from the UWB radio). |
| **overhearing** | Promiscuous reception of frames not addressed to this node, used to sense nearby transmitters. |
| **interference graph** | Graph whose edges mark node pairs that would interfere if they transmitted at once. |
| **conflict graph / graph coloring** | Assigning "colors" (slots) to graph nodes so adjacent (conflicting) nodes get different colors; non-adjacent nodes may reuse a color. |
| **cluster** | A connected group of nodes that must be coordinated together. |
| **lease** | Soft-state ownership/membership that expires unless renewed, so the system self-heals when a node leaves. |

## Distributed optimization

| Term | Meaning |
|---|---|
| **DCOP** | Distributed Constraint Optimization Problem — agents pick local variables to optimize a global objective with only neighbor communication. |
| **MGM** | Maximum-Gain Messaging — a distributed local-search algorithm for DCOP: each round, agents exchange the gain of their best unilateral change and only the largest-gain agent in a neighborhood changes (monotonic, collision-free). |
