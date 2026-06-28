/*
 * superframe.h  (chip-independent, pure)
 *
 * Slot / superframe timing for Tier A / CORE 1st scope. Radio- and Arduino-agnostic.
 * All times are in ms; the caller injects nowMs (usually millis()) -> host unit-testable.
 *
 *   superframe = numSlots slots. length = numSlots * slotLenMs.
 *   slot = [guard | work-window | guard]. The owning anchor transmits TWR only in the work-window.
 *   epoch (slot-phase reference) is injected from gossip sync via setEpoch(). While unsynced every
 *   window stays closed.
 */

#ifndef SUPERFRAME_H
#define SUPERFRAME_H

#include <stdint.h>

struct SuperframeConfig {
    uint8_t  numSlots;   // K_s
    uint16_t slotLenMs;
    uint16_t guardMs;
};

class Superframe {
public:
    Superframe() : _cfg{16, 45, 5}, _epochMs(0), _synced(false) {}

    void begin(const SuperframeConfig& cfg) { _cfg = cfg; _synced = false; }

    // gossip slot-phase sync. epochMs = time of superframe start (slot 0 boundary).
    void setEpoch(uint32_t epochMs) { _epochMs = epochMs; _synced = true; }
    bool synced() const { return _synced; }

    uint32_t superframeLenMs() const {
        return (uint32_t)_cfg.numSlots * _cfg.slotLenMs;
    }

    uint32_t phaseMs(uint32_t nowMs) const {
        if (!_synced) return 0;
        return (uint32_t)(nowMs - _epochMs) % superframeLenMs();
    }

    uint8_t slotIndexNow(uint32_t nowMs) const {
        return (uint8_t)(phaseMs(nowMs) / _cfg.slotLenMs);
    }

    // Is it inside my slot's work-window (both guards excluded)?
    bool isMyWorkWindow(uint32_t nowMs, uint8_t mySlot) const {
        if (!_synced || mySlot >= _cfg.numSlots) return false;
        uint32_t off = phaseMs(nowMs) - (uint32_t)mySlot * _cfg.slotLenMs;
        if (off >= _cfg.slotLenMs) return false;   // unsigned: large value if not my slot
        return off >= _cfg.guardMs && off < (uint32_t)(_cfg.slotLenMs - _cfg.guardMs);
    }

    // Remaining time (ms) in my work-window. 0 if not in my window.
    uint32_t workRemainingMs(uint32_t nowMs, uint8_t mySlot) const {
        if (!isMyWorkWindow(nowMs, mySlot)) return 0;
        uint32_t off = phaseMs(nowMs) - (uint32_t)mySlot * _cfg.slotLenMs;
        return (uint32_t)(_cfg.slotLenMs - _cfg.guardMs) - off;
    }

    // Is it inside another slot's work-window = the overhearing (promiscuous RX) window?
    bool isOthersSlot(uint32_t nowMs, uint8_t mySlot) const {
        if (!_synced) return false;
        uint8_t s = slotIndexNow(nowMs);
        if (s == mySlot) return false;
        uint32_t off = phaseMs(nowMs) - (uint32_t)s * _cfg.slotLenMs;
        return off >= _cfg.guardMs && off < (uint32_t)(_cfg.slotLenMs - _cfg.guardMs);
    }

private:
    SuperframeConfig _cfg;
    uint32_t _epochMs;
    bool     _synced;
};

#endif // SUPERFRAME_H
