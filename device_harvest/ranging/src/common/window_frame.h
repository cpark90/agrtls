/*
 * window_frame.h  (chip-independent, pure)
 *
 * Two-level TDMA timing for window-TDMA (docs/ARCHITECTURE_window_tdma.md §5):
 *   superframe = numWindows windows; each window = slotsPerWindow anchor-slots.
 * Time is injected (nowMs) and the epoch is gossip-synced via setEpoch() -> host-testable.
 *
 * (This is the two-level version; superframe.h remains the slot-only timing used by the current
 *  F-a/F-b meshagent.)
 */

#ifndef WINDOW_FRAME_H
#define WINDOW_FRAME_H

#include <stdint.h>

struct WindowFrameConfig {
    uint8_t  numWindows;
    uint8_t  slotsPerWindow;
    uint16_t slotLenMs;
    uint16_t guardMs;
};

class WindowFrame {
public:
    WindowFrame() : _cfg{2, 6, 20, 2}, _epochMs(0), _synced(false) {}

    void begin(const WindowFrameConfig& cfg) { _cfg = cfg; _synced = false; }
    void setEpoch(uint32_t epochMs) { _epochMs = epochMs; _synced = true; }
    bool synced() const { return _synced; }

    // Number of windows the superframe currently cycles. Driven dynamically from the (shared)
    // coloring so empty windows are never traversed; must stay identical across anchors (derive it
    // only from shared state) or epoch sync drifts. Clamped to >=1 (superframe length must be > 0).
    void setNumWindows(uint8_t n) { _cfg.numWindows = (n == 0) ? 1 : n; }
    uint8_t numWindows() const { return _cfg.numWindows; }

    // Anchor-slots per window. Driven dynamically from the shared registry (= max effective anchors
    // over any tag) so a window has exactly enough slots for its tag's anchors, no more. Must stay
    // identical across anchors (derive from shared state only) or the epoch/slots desync.
    void setSlotsPerWindow(uint8_t n) { _cfg.slotsPerWindow = (n == 0) ? 1 : n; }
    uint8_t slotsPerWindow() const { return _cfg.slotsPerWindow; }

    uint32_t windowLenMs()    const { return (uint32_t)_cfg.slotsPerWindow * _cfg.slotLenMs; }
    uint32_t superframeLenMs() const { return (uint32_t)_cfg.numWindows * windowLenMs(); }

    uint32_t phaseMs(uint32_t nowMs) const {
        if (!_synced) return 0;
        return (uint32_t)(nowMs - _epochMs) % superframeLenMs();
    }

    uint8_t windowIndexNow(uint32_t nowMs) const {
        return (uint8_t)(phaseMs(nowMs) / windowLenMs());
    }
    uint8_t slotIndexNow(uint32_t nowMs) const {
        return (uint8_t)((phaseMs(nowMs) % windowLenMs()) / _cfg.slotLenMs);
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

    // Is it this anchor's turn now: window `windowIdx`, anchor-slot `anchorSlot`, in the work part.
    bool isMyTurn(uint32_t nowMs, uint8_t windowIdx, uint8_t anchorSlot) const {
        return _synced &&
               windowIndexNow(nowMs) == windowIdx &&
               slotIndexNow(nowMs) == anchorSlot &&
               isWork(nowMs);
    }

private:
    WindowFrameConfig _cfg;
    uint32_t _epochMs;
    bool     _synced;
};

#endif // WINDOW_FRAME_H
