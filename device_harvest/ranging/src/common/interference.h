/*
 * interference.h  (칩 독립, 순수)
 *
 * L2 융합 간섭그래프. 두 신호의 합집합으로 간섭 이웃 C(i)를 만든다:
 *   - shared-tag (mesh TAGLIST): 내 태그와 겹치는 앵커
 *   - audible (UWB overhearing + mesh AUDIBLE): 내가 직접 들었거나, 상대가 나를 듣는다고 보고
 * 각 간선은 lastHeard 리스로 에이징(재배치/전원 churn 대응).
 *
 * 단일 채널 1차 가정이므로 N1/N2 구분 없이 "간섭하면 이웃".
 */

#ifndef INTERFERENCE_H
#define INTERFERENCE_H

#include <stdint.h>

#ifndef IG_MAX_NEIGHBORS
#define IG_MAX_NEIGHBORS 12
#endif
#ifndef IG_MAX_TAGS
#define IG_MAX_TAGS 12
#endif

class InterferenceGraph {
public:
    InterferenceGraph()
        : _myId(0), _lease(3600), _thresh(-90.0f), _n(0), _myTagN(0), _idN(0) {}

    void begin(uint16_t myId, uint32_t leaseLenMs, float audibleThreshDbm) {
        _myId = myId; _lease = leaseLenMs; _thresh = audibleThreshDbm;
        _n = 0; _myTagN = 0; _idN = 0;
    }

    void setMyTags(const uint16_t* tagIds, uint8_t n) {
        _myTagN = (n > IG_MAX_TAGS) ? IG_MAX_TAGS : n;
        for (uint8_t i = 0; i < _myTagN; i++) _myTag[i] = tagIds[i];
    }

    // --- inputs ---

    // 로컬 UWB overhearing: 앵커 id 를 rxp 로 들음.
    void onOverheard(uint16_t id, float rxp_dBm, uint32_t nowMs) {
        if (id == _myId || rxp_dBm < _thresh) return;
        Entry* e = entryFor(id);
        if (e) e->lastLocalHear = stamp(nowMs);
    }

    // mesh AUDIBLE: fromId 가 heard[] 를 듣는다고 보고. 그 중 내가 있으면 fromId 가 나를 들음 → 이웃.
    void onAudibleReport(uint16_t fromId, const uint16_t* heard, uint8_t n, uint32_t nowMs) {
        if (fromId == _myId) return;
        for (uint8_t i = 0; i < n; i++) {
            if (heard[i] == _myId) {
                Entry* e = entryFor(fromId);
                if (e) e->lastReportedHearsMe = stamp(nowMs);
                return;
            }
        }
    }

    // mesh TAGLIST: fromId 의 태그집합. 내 태그와 겹치면 shared-tag 간선.
    void onTagList(uint16_t fromId, const uint16_t* tagIds, uint8_t n, uint32_t nowMs) {
        if (fromId == _myId) return;
        for (uint8_t i = 0; i < n; i++)
            for (uint8_t j = 0; j < _myTagN; j++)
                if (tagIds[i] == _myTag[j]) {
                    Entry* e = entryFor(fromId);
                    if (e) e->lastShared = stamp(nowMs);
                    return;
                }
    }

    // 만료 간선 제거 + 이웃 id 리스트 갱신.
    void tick(uint32_t nowMs) {
        uint8_t k = 0;
        for (uint8_t i = 0; i < _n; i++) {
            if (edgeFresh(_e[i], nowMs)) { if (k != i) _e[k] = _e[i]; k++; }
        }
        _n = k;
        _idN = 0;
        for (uint8_t i = 0; i < _n; i++) _ids[_idN++] = _e[i].id;
    }

    // --- outputs ---

    uint8_t         neighborCount() const { return _idN; }
    const uint16_t* neighborIds()   const { return _ids; }

    bool isNeighbor(uint16_t id) const {
        for (uint8_t i = 0; i < _idN; i++) if (_ids[i] == id) return true;
        return false;
    }

    // 내가 로컬로 듣는(fresh) 앵커들 → mesh AUDIBLE 로 게시.
    uint8_t myAudibleList(uint16_t* out, uint8_t maxN, uint32_t nowMs) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n && c < maxN; i++) {
            if (fresh(_e[i].lastLocalHear, nowMs)) out[c++] = _e[i].id;
        }
        return c;
    }

private:
    struct Entry {
        uint16_t id;
        uint32_t lastLocalHear;       // 0 = 없음 (stamp 는 1부터)
        uint32_t lastReportedHearsMe;
        uint32_t lastShared;
    };

    // 0을 "없음"으로 쓰기 위해 타임스탬프를 1-기반으로 저장.
    static uint32_t stamp(uint32_t nowMs) { return nowMs + 1; }

    bool fresh(uint32_t s, uint32_t nowMs) const {
        if (s == 0) return false;
        return (uint32_t)((nowMs + 1) - s) <= _lease;
    }

    bool edgeFresh(const Entry& e, uint32_t nowMs) const {
        return fresh(e.lastLocalHear, nowMs) ||
               fresh(e.lastReportedHearsMe, nowMs) ||
               fresh(e.lastShared, nowMs);
    }

    int find(uint16_t id) const {
        for (uint8_t i = 0; i < _n; i++) if (_e[i].id == id) return (int)i;
        return -1;
    }

    Entry* entryFor(uint16_t id) {
        int i = find(id);
        if (i >= 0) return &_e[i];
        if (_n >= IG_MAX_NEIGHBORS) return nullptr;
        _e[_n] = Entry{id, 0, 0, 0};
        return &_e[_n++];
    }

    uint16_t _myId;
    uint32_t _lease;
    float    _thresh;

    Entry    _e[IG_MAX_NEIGHBORS];
    uint8_t  _n;

    uint16_t _myTag[IG_MAX_TAGS];
    uint8_t  _myTagN;

    uint16_t _ids[IG_MAX_NEIGHBORS];
    uint8_t  _idN;
};

#endif // INTERFERENCE_H
