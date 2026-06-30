/*
 * interference.h  (chip-independent, pure)
 *
 * L2 fused interference graph. Builds the interfering-neighbor set C(i) from the union of two
 * signals:
 *   - shared-tag (mesh TAGLIST): anchors whose tag set overlaps mine
 *   - audible (UWB overhearing + mesh AUDIBLE): anchors I heard directly, or that report hearing me
 * Each edge is lease-aged by lastHeard (handles relocation / power churn).
 *
 * Single-channel 1st-scope assumption: no N1/N2 distinction -> "interferes => neighbor".
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
        : _myId(0), _lease(3600), _thresh(-90.0f), _entryCount(0), _myTagN(0), _idN(0) {}

    void begin(uint16_t myId, uint32_t leaseLenMs, float audibleThreshDbm) {
        _myId = myId; _lease = leaseLenMs; _thresh = audibleThreshDbm;
        _entryCount = 0; _myTagN = 0; _idN = 0;
    }

    void setMyTags(const uint16_t* tagIds, uint8_t n) {
        _myTagN = (n > IG_MAX_TAGS) ? IG_MAX_TAGS : n;
        for (uint8_t i = 0; i < _myTagN; i++) _myTag[i] = tagIds[i];
    }

    // --- inputs ---

    // Local UWB overhearing: heard anchor id at rxp.
    void onOverheard(uint16_t id, float rxp_dBm, uint32_t nowMs) {
        if (id == _myId || rxp_dBm < _thresh) return;
        Entry* entry = entryFor(id);
        if (entry) entry->lastLocalHear = stamp(nowMs);
    }

    // mesh AUDIBLE: fromId reports hearing heard[]. If I am in it, fromId hears me -> neighbor.
    void onAudibleReport(uint16_t fromId, const uint16_t* heard, uint8_t n, uint32_t nowMs) {
        if (fromId == _myId) return;
        for (uint8_t i = 0; i < n; i++) {
            if (heard[i] == _myId) {
                Entry* entry = entryFor(fromId);
                if (entry) entry->lastReportedHearsMe = stamp(nowMs);
                return;
            }
        }
    }

    // mesh TAGLIST: fromId's tag set. If it overlaps mine -> shared-tag edge.
    void onTagList(uint16_t fromId, const uint16_t* tagIds, uint8_t n, uint32_t nowMs) {
        if (fromId == _myId) return;
        for (uint8_t i = 0; i < n; i++)
            for (uint8_t j = 0; j < _myTagN; j++)
                if (tagIds[i] == _myTag[j]) {
                    Entry* entry = entryFor(fromId);
                    if (entry) entry->lastShared = stamp(nowMs);
                    return;
                }
    }

    // Drop expired edges + refresh the neighbor id list.
    void tick(uint32_t nowMs) {
        uint8_t writeIdx = 0;
        for (uint8_t i = 0; i < _entryCount; i++) {
            if (edgeFresh(_entries[i], nowMs)) { if (writeIdx != i) _entries[writeIdx] = _entries[i]; writeIdx++; }
        }
        _entryCount = writeIdx;
        _idN = 0;
        for (uint8_t i = 0; i < _entryCount; i++) _ids[_idN++] = _entries[i].id;
    }

    // --- outputs ---

    uint8_t         neighborCount() const { return _idN; }
    const uint16_t* neighborIds()   const { return _ids; }

    bool isNeighbor(uint16_t id) const {
        for (uint8_t i = 0; i < _idN; i++) if (_ids[i] == id) return true;
        return false;
    }

    // Anchors I currently hear locally (fresh) -> publish as mesh AUDIBLE.
    uint8_t myAudibleList(uint16_t* out, uint8_t maxN, uint32_t nowMs) const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < _entryCount && count < maxN; i++) {
            if (fresh(_entries[i].lastLocalHear, nowMs)) out[count++] = _entries[i].id;
        }
        return count;
    }

private:
    struct Entry {
        uint16_t id;
        uint32_t lastLocalHear;       // 0 = none (stamp is 1-based)
        uint32_t lastReportedHearsMe;
        uint32_t lastShared;
    };

    // Store timestamps 1-based so 0 can mean "none".
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
        for (uint8_t i = 0; i < _entryCount; i++) if (_entries[i].id == id) return (int)i;
        return -1;
    }

    Entry* entryFor(uint16_t id) {
        int i = find(id);
        if (i >= 0) return &_entries[i];
        if (_entryCount >= IG_MAX_NEIGHBORS) return nullptr;
        _entries[_entryCount] = Entry{id, 0, 0, 0};
        return &_entries[_entryCount++];
    }

    uint16_t _myId;
    uint32_t _lease;
    float    _thresh;

    Entry    _entries[IG_MAX_NEIGHBORS];
    uint8_t  _entryCount;

    uint16_t _myTag[IG_MAX_TAGS];
    uint8_t  _myTagN;

    uint16_t _ids[IG_MAX_NEIGHBORS];
    uint8_t  _idN;
};

#endif // INTERFERENCE_H
