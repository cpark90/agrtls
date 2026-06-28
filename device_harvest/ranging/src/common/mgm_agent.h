/*
 * mgm_agent.h  (chip-independent, pure)
 *
 * L3 distributed slot assignment -- single-slot MGM (Maximum Gain Messaging).
 * Mesh/UWB-agnostic: deals only with VALUE/GAIN message structs in/out (no callbacks, pull model).
 *
 * Round has 3 phases: broadcast VALUE -> broadcast GAIN -> DECIDE.
 * DECIDE: if gain delta > 0 and it beats every neighbor's delta (ties -> larger id), move slot.
 *   -> at most one neighbor moves per round -> global conflict is monotonically non-increasing
 *      (no oscillation).
 * The conflict set (neighbors) is injected each round by InterferenceGraph via setNeighbors().
 */

#ifndef MGM_AGENT_H
#define MGM_AGENT_H

#include <stdint.h>

#ifndef MGM_MAX_NEIGHBORS
#define MGM_MAX_NEIGHBORS 12
#endif
#define MGM_NONE 0xFF

enum class MgmState   : uint8_t { SEEKING, HELD };
enum class RoundPhase : uint8_t { IDLE, COLLECT_VALUE, COLLECT_GAIN };

struct MgmConfig {
    uint8_t  numSlots;       // K_s
    uint16_t roundPeriodMs;  // T_round
    uint16_t collectMs;      // T_collect (per-phase collection window)
    uint32_t leaseLenMs;
};

struct ValueMsg { uint16_t round; uint16_t agentId; uint8_t slot; uint32_t leaseExpiry; };
struct GainMsg  { uint16_t round; uint16_t agentId; int16_t  gain; uint8_t propSlot;   };

class MgmAgent {
public:
    MgmAgent()
        : _myId(0), _slot(MGM_NONE), _state(MgmState::SEEKING),
          _phase(RoundPhase::IDLE), _roundNo(0), _nextRoundAt(0), _phaseStart(0),
          _n(0), _proposedSlot(0), _myGain(0),
          _hasOutValue(false), _hasOutGain(false) {
        _cfg = MgmConfig{16, 500, 200, 3600};
    }

    void begin(uint16_t myId, const MgmConfig& cfg) {
        _myId = myId; _cfg = cfg;
        _slot = MGM_NONE; _state = MgmState::SEEKING; _phase = RoundPhase::IDLE;
        _roundNo = 0; _nextRoundAt = 0; _n = 0;
        _hasOutValue = _hasOutGain = false;
    }

    // Refresh membership each round from InterferenceGraph's current neighbor ids
    // (preserves existing slot/gain for ids that persist).
    void setNeighbors(const uint16_t* ids, uint8_t n) {
        NInfo merged[MGM_MAX_NEIGHBORS];
        uint8_t m = 0;
        for (uint8_t i = 0; i < n && m < MGM_MAX_NEIGHBORS; i++) {
            int j = find(ids[i]);
            merged[m++] = (j >= 0) ? _nb[j]
                                   : NInfo{ids[i], MGM_NONE, 0, 0};
        }
        for (uint8_t i = 0; i < m; i++) _nb[i] = merged[i];
        _n = m;
    }

    void onValue(const ValueMsg& msg, uint32_t /*nowMs*/) {
        int j = find(msg.agentId);
        if (j >= 0) _nb[j].slot = msg.slot;
    }

    void onGain(const GainMsg& msg, uint32_t /*nowMs*/) {
        int j = find(msg.agentId);
        if (j >= 0) { _nb[j].gain = msg.gain; _nb[j].gainRound = msg.round; }
    }

    void tick(uint32_t nowMs) {
        switch (_phase) {
        case RoundPhase::IDLE:
            if (nowMs >= _nextRoundAt) {
                _roundNo++;
                if (_slot == MGM_NONE) _slot = bestSlot();   // cold-start
                _outValue = ValueMsg{_roundNo, _myId, _slot, nowMs + _cfg.leaseLenMs};
                _hasOutValue = true;
                _nextRoundAt = nowMs + _cfg.roundPeriodMs;
                _phase = RoundPhase::COLLECT_VALUE;
                _phaseStart = nowMs;
            }
            break;

        case RoundPhase::COLLECT_VALUE:
            if ((uint32_t)(nowMs - _phaseStart) >= _cfg.collectMs) {
                uint8_t cur = conflictAt(_slot);
                _proposedSlot = bestSlot();
                _myGain = (int16_t)((int)cur - (int)conflictAt(_proposedSlot));
                _outGain = GainMsg{_roundNo, _myId, _myGain, _proposedSlot};
                _hasOutGain = true;
                _phase = RoundPhase::COLLECT_GAIN;
                _phaseStart = nowMs;
            }
            break;

        case RoundPhase::COLLECT_GAIN:
            if ((uint32_t)(nowMs - _phaseStart) >= _cfg.collectMs) {
                if (_myGain > 0 && winsContest()) _slot = _proposedSlot;
                _state = (_slot != MGM_NONE && conflictAt(_slot) == 0)
                             ? MgmState::HELD : MgmState::SEEKING;
                _phase = RoundPhase::IDLE;
            }
            break;
        }
    }

    bool pendingValue(ValueMsg& out) {
        if (!_hasOutValue) return false;
        out = _outValue; _hasOutValue = false; return true;
    }
    bool pendingGain(GainMsg& out) {
        if (!_hasOutGain) return false;
        out = _outGain; _hasOutGain = false; return true;
    }

    MgmState state() const { return _state; }
    uint8_t  slot()  const { return _slot; }
    uint8_t  conflictCount() const { return conflictAt(_slot); }
    void     notifyMoved() { _state = MgmState::SEEKING; }

private:
    struct NInfo { uint16_t id; uint8_t slot; int16_t gain; uint16_t gainRound; };

    int find(uint16_t id) const {
        for (uint8_t i = 0; i < _n; i++) if (_nb[i].id == id) return (int)i;
        return -1;
    }

    // Number of neighbors occupying slot s.
    uint8_t conflictAt(uint8_t s) const {
        if (s == MGM_NONE) return 0xFF;  // unassigned = worst
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++)
            if (_nb[i].slot != MGM_NONE && _nb[i].slot == s) c++;
        return c;
    }

    // Least-conflict slot (ties -> lowest index).
    uint8_t bestSlot() const {
        uint8_t best = 0; uint8_t bestC = 0xFF;
        for (uint8_t s = 0; s < _cfg.numSlots; s++) {
            uint8_t c = conflictAt(s);
            if (c < bestC) { bestC = c; best = s; }
        }
        return best;
    }

    // Does my gain beat every neighbor that sent GAIN this round (ties -> larger id wins)?
    bool winsContest() const {
        for (uint8_t i = 0; i < _n; i++) {
            if (_nb[i].gainRound != _roundNo) continue;
            if (_nb[i].gain > _myGain) return false;
            if (_nb[i].gain == _myGain && _nb[i].id > _myId) return false;
        }
        return true;
    }

    uint16_t _myId;
    MgmConfig _cfg;
    uint8_t  _slot;
    MgmState _state;
    RoundPhase _phase;
    uint16_t _roundNo;
    uint32_t _nextRoundAt;
    uint32_t _phaseStart;

    NInfo    _nb[MGM_MAX_NEIGHBORS];
    uint8_t  _n;

    uint8_t  _proposedSlot;
    int16_t  _myGain;

    bool     _hasOutValue;
    bool     _hasOutGain;
    ValueMsg _outValue;
    GainMsg  _outGain;
};

#endif // MGM_AGENT_H
