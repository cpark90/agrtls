# Distributed TDMA — module API

> Function signatures + state tables for the pure, host-testable meshagent modules. Terms:
> [GLOSSARY](GLOSSARY.md). Index: [README](README.md). Surrounding spec:
> [DESIGN_distributed_tdma_core.md](DESIGN_distributed_tdma_core.md).

---

## Shared conventions

- **Location**: these modules (+ `mgm_msg.h`) are meshagent-only and live in the variant folder
  `src/anchor_dw1000_accuracy_meshagent/` (self-contained), NOT in `src/common`. Host test
  `test_p3_modules.cpp` builds with `-I src/common -I src/anchor_dw1000_accuracy_meshagent`.
- **Pure / host-testable**: no `Arduino.h`, no radio. Time injected as `uint32_t nowMs`. I/O via POD message structs (pull model, no callbacks).
- **No dynamic allocation**: fixed-size arrays. `MAX_TAGS = 12` (= `MAX_DEVICES`), `MAX_NEIGHBORS = 12`.
- Sentinels: `PS_NONE = 0xFFFF` (tag addr), `MGM_NONE = 0xFF` (slot).
- The `*_meshagent` variant only wires ESP-NOW/UWB to these modules.

## 1. `superframe.h` — slot/superframe timing

```cpp
struct SuperframeConfig { uint8_t numSlots; uint16_t slotLenMs; uint16_t guardMs; };

class Superframe {
  void     begin(const SuperframeConfig& cfg);
  void     setEpoch(uint32_t epochMs);                 // from gossip slot-phase sync
  bool     synced() const;
  uint32_t superframeLenMs() const;
  uint32_t phaseMs(uint32_t nowMs) const;
  uint8_t  slotIndexNow(uint32_t nowMs) const;
  bool     isMyWorkWindow(uint32_t nowMs, uint8_t mySlot) const;   // my slot, inside work (guards excluded)
  uint32_t workRemainingMs(uint32_t nowMs, uint8_t mySlot) const;  // 0 if not in my frame
  bool     isOthersSlot(uint32_t nowMs, uint8_t mySlot) const;     // overhear frame
};
```

## 2. `peer_scheduler.h` — L4 tag aging scheduler (the original priority request)

```cpp
struct PeerSchedulerConfig { float agingRateBase; float farThreshM; float weakRxpDbm;
                             uint8_t badStreakLimit; float weightMin; };

class PeerScheduler {
  void    begin(const PeerSchedulerConfig& cfg);
  bool    addTag(uint16_t shortAddr);
  bool    removeTag(uint16_t shortAddr);
  uint8_t tagCount() const;
  bool    pick(uint32_t nowMs, uint16_t& outAddr);     // max-score tag (score = (now-lastPolled)*rate)
  void    reportResult(uint16_t addr, uint32_t nowMs, bool ok, float range_m, float rxp_dBm);
  float   demand() const;                              // Σ w(t) → feeds L3
  float   weightOf(uint16_t addr) const;
};
```
Rules: `w(t) = max(weightMin, 1 − badStreak/badStreakLimit)`, `agingRate(t) = agingRateBase·w(t)`. `badStreak` rises while `range>FAR` or `rxp<WEAK` (or on failure), resets on a good sample.

## 3. `interference.h` — fused interference graph + lease

```cpp
class InterferenceGraph {
  void begin(uint16_t myId, uint32_t leaseLenMs, float audibleThreshDbm);
  void setMyTags(const uint16_t* tagIds, uint8_t n);
  void onOverheard(uint16_t anchorId, float rxp_dBm, uint32_t nowMs);                       // local UWB
  void onAudibleReport(uint16_t fromId, const uint16_t* heard, uint8_t n, uint32_t nowMs);  // mesh AUDIBLE
  void onTagList(uint16_t fromId, const uint16_t* tagIds, uint8_t n, uint32_t nowMs);       // mesh TAGLIST
  void tick(uint32_t nowMs);                            // age out edges
  uint8_t         neighborCount() const;
  const uint16_t* neighborIds() const;                 // current C(i)
  bool            isNeighbor(uint16_t id) const;
  uint8_t         myAudibleList(uint16_t* out, uint8_t maxN) const;  // anchors I locally overhear
};
```
Edge `C(i) = sharedTag ∪ audible(i→j) ∪ audible(j→i)`; each source lease-aged independently.

## 4. `mgm_agent.h` — L3 single-slot MGM

```cpp
enum class MgmState  : uint8_t { SEEKING, HELD };
enum class RoundPhase: uint8_t { IDLE, COLLECT_VALUE, COLLECT_GAIN };

struct MgmConfig { uint8_t numSlots; uint16_t roundPeriodMs; uint16_t collectMs; uint32_t leaseLenMs; };
struct ValueMsg  { uint16_t round; uint16_t agentId; uint8_t slot; uint32_t leaseExpiry; };
struct GainMsg   { uint16_t round; uint16_t agentId; int16_t gain;  uint8_t propSlot;   };

class MgmAgent {
  void     begin(uint16_t myId, const MgmConfig& cfg);
  void     setNeighbors(const uint16_t* ids, uint8_t n);
  void     onValue(const ValueMsg& m, uint32_t nowMs);
  void     onGain(const GainMsg& m, uint32_t nowMs);
  void     tick(uint32_t nowMs);
  bool     pendingValue(ValueMsg& out);                // pull outbound
  bool     pendingGain(GainMsg& out);
  MgmState state() const;
  uint8_t  slot() const;
  uint8_t  conflictCount() const;
  void     notifyMoved();
};
```

### State table — RoundPhase
| State | Condition | Action | Next |
|---|---|---|---|
| IDLE | `now ≥ nextRound` | if slot NONE → cold-start pick; stage VALUE; renew lease | COLLECT_VALUE |
| COLLECT_VALUE | `onValue` | update neighbor slot table | COLLECT_VALUE |
| COLLECT_VALUE | `elapsed ≥ collectMs` | compute `slot*`,`Δ`; stage GAIN | COLLECT_GAIN |
| COLLECT_GAIN | `onGain` | update neighbor gain table | COLLECT_GAIN |
| COLLECT_GAIN | `elapsed ≥ collectMs` | DECIDE: `Δ>0 ∧ (Δ,id)>max neighbor` → `slot=slot*` | IDLE |

### State table — MgmState
| State | Condition | Next |
|---|---|---|
| SEEKING | conflictCount()==0 ∧ slot≠NONE (after DECIDE) | HELD |
| HELD | renew lease each round | HELD |
| HELD | `notifyMoved()` or sustained conflict>0 | SEEKING |

## Per-loop wiring order (variant main.cpp)
```
L2:  ingest overheard UWB → ig.onOverheard; ingest mesh → ig.onTagList/onAudibleReport, agent.onValue/onGain
     ig.tick(now); agent.setNeighbors(ig.neighborIds(), ig.neighborCount())
L3:  agent.tick(now); if pendingValue/Gain → mesh.send
     each superframe: mesh.send(TAGLIST), mesh.send(AUDIBLE=ig.myAudibleList())
L4:  if sf.isMyWorkWindow(now, agent.slot()):
        while workRemaining ≥ EXCHANGE_BUDGET and sched.pick(now,t):
           ok,r,p = twr_poll(t); if ok: logRange(idOf(t),r,p); sched.reportResult(t,now,ok,r,p)
     elif sf.isOthersSlot(now, agent.slot()): uwb.promiscuousReceive()
```
