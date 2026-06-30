/*
 * tag_registry.h  (chip-independent, pure)
 *
 * Merges per-anchor measurements (anchorId, tagId, rxPower, range) — the data anchors share over the
 * mesh — into the structure the synchronous TDMA coloring needs (docs/ARCHITECTURE_synchronous_tdma.md §4):
 *
 *   - effectiveAnchors(tag)         : anchors whose link to the tag is eligible (q >= theta_link)
 *   - conflict(tagA, tagB)          : the two tags share an EFFECTIVE anchor (both ranged by it)
 *   - meanQuality(tag)              : per-tag priority = mean linkQuality over its effective anchors
 *   - meanSharedQuality(tag, other) : mean linkQuality of `tag` over anchors effective for BOTH
 *                                     (count-normalized, calibration-robust pairwise comparison)
 *
 * Pure / host-testable / no allocation (fixed arrays). One entry per (anchor, tag) pair.
 */

#ifndef TAG_REGISTRY_H
#define TAG_REGISTRY_H

#include <stdint.h>
#include "tag_quality.h"

#ifndef TR_MAX_ENTRIES
#define TR_MAX_ENTRIES 96      // (anchor, tag) pairs
#endif
#ifndef TR_MAX_TAGS
#define TR_MAX_TAGS 24
#endif
#ifndef TR_ENTRY_TTL_MS
#define TR_ENTRY_TTL_MS 5000   // drop a link not refreshed within this (vanished link -> stale entry)
#endif

class TagRegistry {
public:
    void begin() { _entryCount = 0; }

    // Add or update the measurement for an (anchor, tag) link. The stored rxPower is EMA-smoothed and
    // the eligibility is hysteretic (tag_quality.h) so a single noisy/marginal sample does not flip the
    // schedule (which would desync the frame epoch across anchors). nowMs timestamps the link for
    // aging via expire().
    void report(uint16_t anchorId, uint16_t tagId, float rxPower, float range, uint32_t nowMs = 0) {
        for (uint8_t i = 0; i < _entryCount; i++) {
            if (_entries[i].anchorId == anchorId && _entries[i].tagId == tagId) {
                _entries[i].rxPower  = qualityEma(_entries[i].rxPower, rxPower);  // first-cut: quality == rxPower
                _entries[i].range    = range;
                _entries[i].eligible = linkEligibleHyst(linkQuality(_entries[i].rxPower, _entries[i].range), _entries[i].eligible);
                _entries[i].lastSeenMs = nowMs;
                return;
            }
        }
        if (_entryCount < TR_MAX_ENTRIES) {
            bool eligible = linkEligibleHyst(linkQuality(rxPower, range), false);   // join only if clearly good
            _entries[_entryCount++] = Entry{anchorId, tagId, rxPower, range, eligible, nowMs};
        }
    }

    // Apply a PEER anchor's published link. The peer's eligibility is authoritative (it owns the link
    // and decided it from its own smoothed history) -> store verbatim, do NOT recompute locally. This
    // is what keeps every anchor's view of each link identical (consistent coloring / slot counts).
    // Use report() (above) only for THIS anchor's own measurements.
    void reportPeer(uint16_t anchorId, uint16_t tagId, float rxPower, float range, bool eligible, uint32_t nowMs = 0) {
        for (uint8_t i = 0; i < _entryCount; i++)
            if (_entries[i].anchorId == anchorId && _entries[i].tagId == tagId) {
                _entries[i].rxPower = rxPower; _entries[i].range = range;
                _entries[i].eligible = eligible; _entries[i].lastSeenMs = nowMs;
                return;
            }
        if (_entryCount < TR_MAX_ENTRIES)
            _entries[_entryCount++] = Entry{anchorId, tagId, rxPower, range, eligible, nowMs};
    }

    // Extract THIS anchor's own links (smoothed rxPower + its eligibility decision) for publishing over
    // the mesh. Parallel arrays keep this module free of the wire/message type. Returns the count.
    uint8_t selfLinks(uint16_t anchorId, uint16_t* tagId, float* rxPower, float* range, bool* eligible, uint8_t maxN) const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < _entryCount && count < maxN; i++)
            if (_entries[i].anchorId == anchorId) {
                tagId[count]    = _entries[i].tagId;
                rxPower[count]  = _entries[i].rxPower;
                range[count]    = _entries[i].range;
                eligible[count] = _entries[i].eligible;
                count++;
            }
        return count;
    }

    // Drop links not refreshed within ttlMs (a vanished anchor-tag link must not linger as a stale
    // entry that keeps a gone tag "active" / inflates slot counts). Entries reported with nowMs==0
    // (the default, e.g. host tests not exercising aging) are never expired.
    void expire(uint32_t nowMs, uint32_t ttlMs = TR_ENTRY_TTL_MS) {
        uint8_t writeIdx = 0;
        for (uint8_t i = 0; i < _entryCount; i++) {
            bool stale = _entries[i].lastSeenMs != 0 && (uint32_t)(nowMs - _entries[i].lastSeenMs) > ttlMs;
            if (!stale) { if (writeIdx != i) _entries[writeIdx] = _entries[i]; writeIdx++; }
        }
        _entryCount = writeIdx;
    }

    // Distinct tag ids seen.
    uint8_t tags(uint16_t* out, uint8_t maxN) const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < _entryCount; i++) {
            uint16_t tagId = _entries[i].tagId;
            bool seen = false;
            for (uint8_t j = 0; j < count; j++) if (out[j] == tagId) { seen = true; break; }
            if (!seen && count < maxN) out[count++] = tagId;
        }
        return count;
    }

    // Anchors with an eligible link to the tag.
    uint8_t effectiveAnchors(uint16_t tagId, uint16_t* out, uint8_t maxN) const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < _entryCount && count < maxN; i++)
            if (_entries[i].tagId == tagId && _entries[i].eligible) out[count++] = _entries[i].anchorId;
        return count;
    }

    uint8_t effectiveAnchorCount(uint16_t tagId) const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < _entryCount; i++)
            if (_entries[i].tagId == tagId && _entries[i].eligible) count++;
        return count;
    }

    // Does anchorId currently have an eligible link to tagId? Used to decide whether an anchor still
    // needs to (boot)probe a tag (it is NOT yet effective) vs. range it in the tag's active frame.
    bool isEffectiveAnchor(uint16_t tagId, uint16_t anchorId) const {
        for (uint8_t i = 0; i < _entryCount; i++)
            if (_entries[i].tagId == tagId && _entries[i].anchorId == anchorId)
                return _entries[i].eligible;
        return false;
    }

    // Anchor-slot coloring (deterministic, same on every anchor from the shared registry): the slot
    // an anchor takes within tagId's frame = its RANK among tagId's effective anchors ordered by id.
    // Two anchors ranging the same tag thus get distinct slots (collision-free) with no negotiation.
    // Returns 0xFF if anchorId is not an effective anchor of tagId.
    uint8_t anchorRank(uint16_t tagId, uint16_t anchorId) const {
        if (!isEffectiveAnchor(tagId, anchorId)) return 0xFF;
        uint8_t rank = 0;
        for (uint8_t i = 0; i < _entryCount; i++)
            if (_entries[i].tagId == tagId && _entries[i].eligible && _entries[i].anchorId < anchorId) rank++;
        return rank;
    }

    // Max effective anchors over any single tag -> the number of anchor-slots a frame needs so all
    // effective anchors of its tag fit (drives the dynamic slotsPerFrame). >=0.
    uint8_t maxEffectiveAnchorCount() const {
        uint16_t seen[TR_MAX_TAGS]; uint8_t seenCount = 0, maxCount = 0;
        for (uint8_t i = 0; i < _entryCount; i++) {
            uint16_t tagId = _entries[i].tagId;
            bool done = false;
            for (uint8_t j = 0; j < seenCount; j++) if (seen[j] == tagId) { done = true; break; }
            if (done) continue;
            if (seenCount < TR_MAX_TAGS) seen[seenCount++] = tagId;
            uint8_t count = effectiveAnchorCount(tagId);
            if (count > maxCount) maxCount = count;
        }
        return maxCount;
    }

    // Lowest-id anchor that holds ANY eligible link = the cluster's epoch master (timing is driven by a
    // participating/effective anchor, not a weak lowest-id node). 0xFFFF if no anchor is eligible yet.
    uint16_t lowestEffectiveAnchor() const {
        uint16_t lo = 0xFFFF;
        for (uint8_t i = 0; i < _entryCount; i++)
            if (_entries[i].eligible && _entries[i].anchorId < lo) lo = _entries[i].anchorId;
        return lo;
    }

    // Two tags conflict iff some anchor ranges both effectively.
    bool conflict(uint16_t tagA, uint16_t tagB) const {
        for (uint8_t i = 0; i < _entryCount; i++) {
            if (_entries[i].tagId != tagA || !_entries[i].eligible) continue;
            for (uint8_t j = 0; j < _entryCount; j++)
                if (_entries[j].tagId == tagB && _entries[j].anchorId == _entries[i].anchorId &&
                    _entries[j].eligible) return true;
        }
        return false;
    }

    // Per-tag priority for scheduling: mean linkQuality over the tag's effective anchors.
    // Returns -1e9 if the tag has no effective anchor (a candidate).
    float meanQuality(uint16_t tagId) const {
        float sum = 0; uint8_t count = 0;
        for (uint8_t i = 0; i < _entryCount; i++)
            if (_entries[i].tagId == tagId && _entries[i].eligible) {
                sum += linkQuality(_entries[i].rxPower, _entries[i].range); count++;
            }
        return count ? sum / count : -1e9f;
    }

    // Pairwise frame-priority metric: mean linkQuality of `tagId` over the anchors that range BOTH
    // `tagId` and `otherTagId` effectively (the shared effective anchors). -1e9 if none.
    float meanSharedQuality(uint16_t tagId, uint16_t otherTagId) const {
        float sum = 0; uint8_t count = 0;
        for (uint8_t i = 0; i < _entryCount; i++) {
            if (_entries[i].tagId != tagId || !_entries[i].eligible) continue;
            bool shared = false;
            for (uint8_t j = 0; j < _entryCount; j++)
                if (_entries[j].tagId == otherTagId && _entries[j].anchorId == _entries[i].anchorId &&
                    _entries[j].eligible) { shared = true; break; }
            if (shared) { sum += linkQuality(_entries[i].rxPower, _entries[i].range); count++; }
        }
        return count ? sum / count : -1e9f;
    }

private:
    struct Entry {
        uint16_t anchorId;
        uint16_t tagId;
        float    rxPower;
        float    range;
        bool     eligible;
        uint32_t lastSeenMs;
    };
    Entry   _entries[TR_MAX_ENTRIES];
    uint8_t _entryCount = 0;
};

#endif // TAG_REGISTRY_H
