/*
 * superframe.h  (칩 독립, 순수)
 *
 * Tier A / CORE 1차의 슬롯·superframe 타이밍 계산. 라디오/Arduino 무관.
 * 모든 시간은 ms이며 호출자가 nowMs(보통 millis())를 주입 → 호스트 단위테스트 가능.
 *
 *   superframe = numSlots 개 슬롯. 길이 = numSlots * slotLenMs.
 *   슬롯 = [guard | work-window | guard]. 소유 앵커는 work-window에서만 TWR 송신.
 *   epoch(슬롯 위상 기준)은 gossip 동기 결과를 setEpoch()로 주입. 미동기면 모든 창이 닫힘.
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

    // gossip 슬롯 위상 동기. epochMs = superframe 시작(슬롯0 경계) 시각.
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

    // 내 슬롯의 work-window 안인가 (양끝 guard 제외).
    bool isMyWorkWindow(uint32_t nowMs, uint8_t mySlot) const {
        if (!_synced || mySlot >= _cfg.numSlots) return false;
        uint32_t off = phaseMs(nowMs) - (uint32_t)mySlot * _cfg.slotLenMs;
        if (off >= _cfg.slotLenMs) return false;   // 부호없음: 내 슬롯이 아니면 큰 값
        return off >= _cfg.guardMs && off < (uint32_t)(_cfg.slotLenMs - _cfg.guardMs);
    }

    // 내 work-window 잔여 시간(ms). 내 창이 아니면 0.
    uint32_t workRemainingMs(uint32_t nowMs, uint8_t mySlot) const {
        if (!isMyWorkWindow(nowMs, mySlot)) return 0;
        uint32_t off = phaseMs(nowMs) - (uint32_t)mySlot * _cfg.slotLenMs;
        return (uint32_t)(_cfg.slotLenMs - _cfg.guardMs) - off;
    }

    // 다른 슬롯의 work-window 안인가 = overhearing(promiscuous RX) 창.
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
