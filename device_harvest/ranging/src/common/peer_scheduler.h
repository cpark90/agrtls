/*
 * peer_scheduler.h  (chip-independent, pure)
 *
 * L4 intra-anchor tag scheduler. Decides "which tag to poll inside my slot".
 * Implements the original request (penalty per update + lower priority for far/weak tags).
 *
 * score = (now - lastPolled) * agingRate(t)  (a tag that waited longer ranks higher).
 * After polling, lastPolled = now (the penalty -> sent to the back).
 * agingRate(t) = agingRateBase * w(t), w(t) = max(weightMin, 1 - badStreak/badStreakLimit).
 *   -> far/weak tags (badStreak up) ramp up slowly and are polled less often.
 * demand = sum of w(t) is the input to L3 (inter-anchor demand split).
 */

#ifndef PEER_SCHEDULER_H
#define PEER_SCHEDULER_H

#include <stdint.h>

#ifndef PS_MAX_TAGS
#define PS_MAX_TAGS 12
#endif
#define PS_NONE 0xFFFF

struct PeerSchedulerConfig {
    float   agingRateBase;   // score per ms
    float   farThreshM;      // FAR
    float   weakRxpDbm;      // WEAK
    uint8_t badStreakLimit;  // weight floor reached at this streak
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

    // Copy current tag short addresses out (for publishing TAGLIST over the mesh).
    uint8_t tagIds(uint16_t* out, uint8_t maxN) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n && c < maxN; i++) out[c++] = _t[i].addr;
        return c;
    }

    // Pick the max-score tag (does not mutate). Report the poll result via reportResult.
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

    // Apply the TWR result. lastPolled is updated regardless of success (prevents a failing
    // tag from monopolizing the schedule).
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
            bad = true;  // a failed exchange counts as a far/weak symptom
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
