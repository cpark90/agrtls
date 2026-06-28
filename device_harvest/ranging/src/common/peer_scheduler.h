/*
 * peer_scheduler.h  (칩 독립, 순수)
 *
 * L4 앵커 내 태그 스케줄러. "내 슬롯 안에서 어느 태그를 폴할지" 결정.
 * 최초 요청(업데이트마다 페널티 + 먼/약한 태그 우선순위 하향)을 그대로 구현.
 *
 * 점수 = (now - lastPolled) * agingRate(t)  (오래 기다린 태그가 높음).
 * 폴 후 lastPolled=now 로 리셋(=페널티, 뒤로 밀림).
 * agingRate(t) = agingRateBase * w(t), w(t)=max(weightMin, 1 - badStreak/badStreakLimit).
 *   → 먼/약한(badStreak↑) 태그는 천천히 차올라 덜 폴린다.
 * demand = Σ w(t) 는 L3(앵커 간 수요배분) 입력.
 */

#ifndef PEER_SCHEDULER_H
#define PEER_SCHEDULER_H

#include <stdint.h>

#ifndef PS_MAX_TAGS
#define PS_MAX_TAGS 12
#endif
#define PS_NONE 0xFFFF

struct PeerSchedulerConfig {
    float   agingRateBase;   // 점수/ms
    float   farThreshM;      // FAR
    float   weakRxpDbm;      // WEAK
    uint8_t badStreakLimit;  // 이 값에서 가중치 바닥
    float   weightMin;       // w_min
};

class PeerScheduler {
public:
    PeerScheduler() : _n(0) {
        _cfg = PeerSchedulerConfig{0.001f, 8.0f, -82.0f, 5, 0.2f};
    }

    void begin(const PeerSchedulerConfig& cfg) { _cfg = cfg; _n = 0; }

    bool addTag(uint16_t addr) {
        if (indexOf(addr) >= 0) return true;
        if (_n >= PS_MAX_TAGS) return false;
        _t[_n] = Tag{addr, 0u, 0u, 0.0f, 0.0f};
        _n++;
        return true;
    }

    bool removeTag(uint16_t addr) {
        int i = indexOf(addr);
        if (i < 0) return false;
        _t[i] = _t[_n - 1];
        _n--;
        return true;
    }

    uint8_t tagCount() const { return _n; }

    // 최대 점수 태그 선택(상태 비변경). 폴 결과는 reportResult 로 반영.
    bool pick(uint32_t nowMs, uint16_t& outAddr) const {
        if (_n == 0) return false;
        int best = -1;
        float bestScore = -1.0f;
        for (uint8_t i = 0; i < _n; i++) {
            float score = (float)(uint32_t)(nowMs - _t[i].lastPolled) * agingRate(i);
            if (score > bestScore) { bestScore = score; best = (int)i; }
        }
        outAddr = _t[best].addr;
        return true;
    }

    // TWR 결과 반영. 성공/실패 무관 lastPolled 갱신(실패 태그 독점 방지).
    void reportResult(uint16_t addr, uint32_t nowMs, bool ok, float range_m, float rxp_dBm) {
        int i = indexOf(addr);
        if (i < 0) return;
        _t[i].lastPolled = nowMs;
        bool bad;
        if (ok) {
            _t[i].lastRange = range_m;
            _t[i].lastRxp   = rxp_dBm;
            bad = (range_m > _cfg.farThreshM) || (rxp_dBm < _cfg.weakRxpDbm);
        } else {
            bad = true;  // 실패는 먼/약한 정황으로 간주
        }
        if (bad) {
            if (_t[i].badStreak < _cfg.badStreakLimit) _t[i].badStreak++;
        } else {
            _t[i].badStreak = 0;
        }
    }

    float demand() const {
        float d = 0.0f;
        for (uint8_t i = 0; i < _n; i++) d += weight(i);
        return d;
    }

    float weightOf(uint16_t addr) const {
        int i = indexOf(addr);
        return (i < 0) ? 0.0f : weight((uint8_t)i);
    }

private:
    struct Tag {
        uint16_t addr;
        uint32_t lastPolled;
        uint8_t  badStreak;
        float    lastRange;
        float    lastRxp;
    };

    int indexOf(uint16_t addr) const {
        for (uint8_t i = 0; i < _n; i++) if (_t[i].addr == addr) return (int)i;
        return -1;
    }

    float weight(uint8_t i) const {
        float w = 1.0f - (float)_t[i].badStreak / (float)_cfg.badStreakLimit;
        return (w < _cfg.weightMin) ? _cfg.weightMin : w;
    }

    float agingRate(uint8_t i) const { return _cfg.agingRateBase * weight(i); }

    PeerSchedulerConfig _cfg;
    Tag     _t[PS_MAX_TAGS];
    uint8_t _n;
};

#endif // PEER_SCHEDULER_H
