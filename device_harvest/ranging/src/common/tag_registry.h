/*
 * tag_registry.h  (chip-independent, pure)
 *
 * Merges per-anchor measurements (anchorId, tagId, rxp, range) — the data anchors share over the
 * mesh — into the structure the window-TDMA coloring needs (docs/ARCHITECTURE_window_tdma.md §4):
 *
 *   - effectiveAnchors(tag)        : anchors whose link to the tag is eligible (q >= theta_link)
 *   - conflict(t1, t2)             : the two tags share an EFFECTIVE anchor (both ranged by it)
 *   - meanQuality(tag)             : per-tag priority = mean linkQuality over its effective anchors
 *   - meanSharedQuality(tag, other): mean linkQuality of `tag` over anchors effective for BOTH
 *                                    (count-normalized, calibration-robust pairwise comparison)
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
    void begin() { _n = 0; }

    // Add or update the measurement for an (anchor, tag) link. The stored rxp is EMA-smoothed and the
    // eligibility is hysteretic (tag_quality.h) so a single noisy/marginal sample does not flip the
    // schedule (which would desync the window epoch across anchors). nowMs timestamps the link for
    // aging via expire().
    void report(uint16_t anchorId, uint16_t tagId, float rxp, float range, uint32_t nowMs = 0) {
        for (uint8_t i = 0; i < _n; i++) {
            if (_e[i].anchor == anchorId && _e[i].tag == tagId) {
                _e[i].rxp   = qualityEma(_e[i].rxp, rxp);   // first-cut: quality == rxp -> smooth rxp
                _e[i].range = range;
                _e[i].eligible = linkEligibleHyst(linkQuality(_e[i].rxp, _e[i].range), _e[i].eligible);
                _e[i].lastMs = nowMs;
                return;
            }
        }
        if (_n < TR_MAX_ENTRIES) {
            bool elig = linkEligibleHyst(linkQuality(rxp, range), false);   // join only if clearly good
            _e[_n++] = Entry{anchorId, tagId, rxp, range, elig, nowMs};
        }
    }

    // Apply a PEER anchor's published link. The peer's eligibility is authoritative (it owns the link
    // and decided it from its own smoothed history) -> store verbatim, do NOT recompute locally. This
    // is what keeps every anchor's view of each link identical (consistent coloring / slot counts).
    // Use report() (above) only for THIS anchor's own measurements.
    void reportPeer(uint16_t anchorId, uint16_t tagId, float rxp, float range, bool eligible, uint32_t nowMs = 0) {
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].anchor == anchorId && _e[i].tag == tagId) {
                _e[i].rxp = rxp; _e[i].range = range; _e[i].eligible = eligible; _e[i].lastMs = nowMs;
                return;
            }
        if (_n < TR_MAX_ENTRIES) _e[_n++] = Entry{anchorId, tagId, rxp, range, eligible, nowMs};
    }

    // Extract THIS anchor's own links (smoothed rxp + its eligibility decision) for publishing over the
    // mesh. Parallel arrays keep this module free of the wire/message type. Returns the count.
    uint8_t selfLinks(uint16_t anchorId, uint16_t* tag, float* rxp, float* range, bool* elig, uint8_t maxN) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n && c < maxN; i++)
            if (_e[i].anchor == anchorId) {
                tag[c] = _e[i].tag; rxp[c] = _e[i].rxp; range[c] = _e[i].range; elig[c] = _e[i].eligible; c++;
            }
        return c;
    }

    // Drop links not refreshed within ttlMs (a vanished anchor-tag link must not linger as a stale
    // entry that keeps a gone tag "active" / inflates slot counts). Entries reported with nowMs==0
    // (the default, e.g. host tests not exercising aging) are never expired.
    void expire(uint32_t nowMs, uint32_t ttlMs = TR_ENTRY_TTL_MS) {
        uint8_t w = 0;
        for (uint8_t i = 0; i < _n; i++) {
            bool stale = _e[i].lastMs != 0 && (uint32_t)(nowMs - _e[i].lastMs) > ttlMs;
            if (!stale) { if (w != i) _e[w] = _e[i]; w++; }
        }
        _n = w;
    }

    // Distinct tag ids seen.
    uint8_t tags(uint16_t* out, uint8_t maxN) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++) {
            uint16_t t = _e[i].tag;
            bool seen = false;
            for (uint8_t j = 0; j < c; j++) if (out[j] == t) { seen = true; break; }
            if (!seen && c < maxN) out[c++] = t;
        }
        return c;
    }

    // Anchors with an eligible link to the tag.
    uint8_t effectiveAnchors(uint16_t tagId, uint16_t* out, uint8_t maxN) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n && c < maxN; i++)
            if (_e[i].tag == tagId && _e[i].eligible) out[c++] = _e[i].anchor;
        return c;
    }

    uint8_t effectiveAnchorCount(uint16_t tagId) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].tag == tagId && _e[i].eligible) c++;
        return c;
    }

    // Does anchorId currently have an eligible link to tagId? Used to decide whether an anchor still
    // needs to (boot)probe a tag (it is NOT yet effective) vs. range it in the tag's active window.
    bool isEffectiveAnchor(uint16_t tagId, uint16_t anchorId) const {
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].tag == tagId && _e[i].anchor == anchorId)
                return _e[i].eligible;
        return false;
    }

    // Anchor-slot coloring (deterministic, same on every anchor from the shared registry): the slot
    // an anchor takes within tagId's window = its RANK among tagId's effective anchors ordered by id.
    // Two anchors ranging the same tag thus get distinct slots (collision-free) with no negotiation.
    // Returns 0xFF if anchorId is not an effective anchor of tagId.
    uint8_t anchorRank(uint16_t tagId, uint16_t anchorId) const {
        if (!isEffectiveAnchor(tagId, anchorId)) return 0xFF;
        uint8_t rank = 0;
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].tag == tagId && _e[i].eligible && _e[i].anchor < anchorId) rank++;
        return rank;
    }

    // Max effective anchors over any single tag -> the number of anchor-slots a window needs so all
    // effective anchors of its tag fit (drives the dynamic slotsPerWindow). >=0.
    uint8_t maxEffectiveAnchorCount() const {
        uint16_t seen[TR_MAX_TAGS]; uint8_t ns = 0, mx = 0;
        for (uint8_t i = 0; i < _n; i++) {
            uint16_t t = _e[i].tag;
            bool done = false;
            for (uint8_t j = 0; j < ns; j++) if (seen[j] == t) { done = true; break; }
            if (done) continue;
            if (ns < TR_MAX_TAGS) seen[ns++] = t;
            uint8_t c = effectiveAnchorCount(t);
            if (c > mx) mx = c;
        }
        return mx;
    }

    // Two tags conflict iff some anchor ranges both effectively.
    bool conflict(uint16_t t1, uint16_t t2) const {
        for (uint8_t i = 0; i < _n; i++) {
            if (_e[i].tag != t1 || !_e[i].eligible) continue;
            for (uint8_t j = 0; j < _n; j++)
                if (_e[j].tag == t2 && _e[j].anchor == _e[i].anchor &&
                    _e[j].eligible) return true;
        }
        return false;
    }

    // Per-tag priority for scheduling: mean linkQuality over the tag's effective anchors.
    // Returns -1e9 if the tag has no effective anchor (a candidate).
    float meanQuality(uint16_t tagId) const {
        float sum = 0; uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].tag == tagId && _e[i].eligible) {
                sum += linkQuality(_e[i].rxp, _e[i].range); c++;
            }
        return c ? sum / c : -1e9f;
    }

    // Pairwise window-priority metric: mean linkQuality of `tag` over the anchors that range BOTH
    // `tag` and `other` effectively (the shared effective anchors). -1e9 if none.
    float meanSharedQuality(uint16_t tag, uint16_t other) const {
        float sum = 0; uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++) {
            if (_e[i].tag != tag || !_e[i].eligible) continue;
            bool shared = false;
            for (uint8_t j = 0; j < _n; j++)
                if (_e[j].tag == other && _e[j].anchor == _e[i].anchor &&
                    _e[j].eligible) { shared = true; break; }
            if (shared) { sum += linkQuality(_e[i].rxp, _e[i].range); c++; }
        }
        return c ? sum / c : -1e9f;
    }

private:
    struct Entry { uint16_t anchor; uint16_t tag; float rxp; float range; bool eligible; uint32_t lastMs; };
    Entry   _e[TR_MAX_ENTRIES];
    uint8_t _n = 0;
};

#endif // TAG_REGISTRY_H
