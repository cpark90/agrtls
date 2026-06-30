/*
 * frame_schedule.h  (chip-independent, pure)
 *
 * Two-level TDMA timing for synchronous TDMA (docs/ARCHITECTURE_synchronous_tdma.md §5):
 *   superframe = numFrames frames; each frame = slotsPerFrame anchor-slots.
 * Time is injected (nowMs) and the epoch is gossip-synced via setEpoch() -> host-testable.
 *
 * (This is the two-level version; superframe.h remains the slot-only timing used by the current
 *  F-a/F-b meshagent.)
 */

#ifndef FRAME_SCHEDULE_H
#define FRAME_SCHEDULE_H

#include <stdint.h>

struct FrameScheduleConfig {
    uint8_t  numFrames;
    uint8_t  slotsPerFrame;
    uint16_t slotLenMs;
    uint16_t guardMs;
};

class FrameSchedule {
public:
    FrameSchedule() : _cfg{2, 6, 20, 2}, _epochMs(0), _synced(false) {}

    void begin(const FrameScheduleConfig& cfg) { _cfg = cfg; _synced = false; }
    void setEpoch(uint32_t epochMs) { _epochMs = epochMs; _synced = true; }
    bool synced() const { return _synced; }

    // Number of frames the superframe currently cycles. Driven dynamically from the (shared)
    // coloring so empty frames are never traversed; must stay identical across anchors (derive it
    // only from shared state) or epoch sync drifts. Clamped to >=1 (superframe length must be > 0).
    void setNumFrames(uint8_t n) { _cfg.numFrames = (n == 0) ? 1 : n; }
    uint8_t numFrames() const { return _cfg.numFrames; }

    // Anchor-slots per frame. Driven dynamically from the shared registry (= max effective anchors
    // over any tag) so a frame has exactly enough slots for its tag's anchors, no more. Must stay
    // identical across anchors (derive from shared state only) or the epoch/slots desync.
    void setSlotsPerFrame(uint8_t n) { _cfg.slotsPerFrame = (n == 0) ? 1 : n; }
    uint8_t slotsPerFrame() const { return _cfg.slotsPerFrame; }

    uint32_t frameLenMs()    const { return (uint32_t)_cfg.slotsPerFrame * _cfg.slotLenMs; }
    uint32_t superframeLenMs() const { return (uint32_t)_cfg.numFrames * frameLenMs(); }

    uint32_t phaseMs(uint32_t nowMs) const {
        if (!_synced) return 0;
        return (uint32_t)(nowMs - _epochMs) % superframeLenMs();
    }

    uint8_t frameIndexNow(uint32_t nowMs) const {
        return (uint8_t)(phaseMs(nowMs) / frameLenMs());
    }
    uint8_t slotIndexNow(uint32_t nowMs) const {
        return (uint8_t)((phaseMs(nowMs) % frameLenMs()) / _cfg.slotLenMs);
    }
    // Monotonic slot instance id (does not wrap) — lets the caller poll at most once per slot.
    uint32_t slotNumber(uint32_t nowMs) const {
        if (!_synced) return 0;
        return (uint32_t)(nowMs - _epochMs) / _cfg.slotLenMs;
    }

    // Within the work part of the current anchor-slot (guards at both ends excluded)?
    bool isWork(uint32_t nowMs) const {
        if (!_synced) return false;
        uint32_t off = phaseMs(nowMs) % _cfg.slotLenMs;
        return off >= _cfg.guardMs && off < (uint32_t)(_cfg.slotLenMs - _cfg.guardMs);
    }

    // Is it this anchor's turn now: frame `frameIdx`, anchor-slot `anchorSlot`, in the work part.
    bool isMyTurn(uint32_t nowMs, uint8_t frameIdx, uint8_t anchorSlot) const {
        return _synced &&
               frameIndexNow(nowMs) == frameIdx &&
               slotIndexNow(nowMs) == anchorSlot &&
               isWork(nowMs);
    }

private:
    FrameScheduleConfig _cfg;
    uint32_t _epochMs;
    bool     _synced;
};

#endif // FRAME_SCHEDULE_H
