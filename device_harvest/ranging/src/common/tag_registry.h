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
#define TR_NONE 0xFFFF

class TagRegistry {
public:
    void begin() { _n = 0; }

    // Add or update the measurement for an (anchor, tag) link.
    void report(uint16_t anchorId, uint16_t tagId, float rxp, float range) {
        for (uint8_t i = 0; i < _n; i++) {
            if (_e[i].anchor == anchorId && _e[i].tag == tagId) {
                _e[i].rxp = rxp; _e[i].range = range; return;
            }
        }
        if (_n < TR_MAX_ENTRIES) _e[_n++] = Entry{anchorId, tagId, rxp, range};
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
            if (_e[i].tag == tagId && linkEligible(_e[i].rxp, _e[i].range)) out[c++] = _e[i].anchor;
        return c;
    }

    uint8_t effectiveAnchorCount(uint16_t tagId) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].tag == tagId && linkEligible(_e[i].rxp, _e[i].range)) c++;
        return c;
    }

    // Does anchorId currently have an eligible link to tagId? Used to decide whether an anchor still
    // needs to (boot)probe a tag (it is NOT yet effective) vs. range it in the tag's active window.
    bool isEffectiveAnchor(uint16_t tagId, uint16_t anchorId) const {
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].tag == tagId && _e[i].anchor == anchorId)
                return linkEligible(_e[i].rxp, _e[i].range);
        return false;
    }

    // Two tags conflict iff some anchor ranges both effectively.
    bool conflict(uint16_t t1, uint16_t t2) const {
        for (uint8_t i = 0; i < _n; i++) {
            if (_e[i].tag != t1 || !linkEligible(_e[i].rxp, _e[i].range)) continue;
            for (uint8_t j = 0; j < _n; j++)
                if (_e[j].tag == t2 && _e[j].anchor == _e[i].anchor &&
                    linkEligible(_e[j].rxp, _e[j].range)) return true;
        }
        return false;
    }

    // Per-tag priority for scheduling: mean linkQuality over the tag's effective anchors.
    // Returns -1e9 if the tag has no effective anchor (a candidate).
    float meanQuality(uint16_t tagId) const {
        float sum = 0; uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++)
            if (_e[i].tag == tagId && linkEligible(_e[i].rxp, _e[i].range)) {
                sum += linkQuality(_e[i].rxp, _e[i].range); c++;
            }
        return c ? sum / c : -1e9f;
    }

    // Pairwise window-priority metric: mean linkQuality of `tag` over the anchors that range BOTH
    // `tag` and `other` effectively (the shared effective anchors). -1e9 if none.
    float meanSharedQuality(uint16_t tag, uint16_t other) const {
        float sum = 0; uint8_t c = 0;
        for (uint8_t i = 0; i < _n; i++) {
            if (_e[i].tag != tag || !linkEligible(_e[i].rxp, _e[i].range)) continue;
            bool shared = false;
            for (uint8_t j = 0; j < _n; j++)
                if (_e[j].tag == other && _e[j].anchor == _e[i].anchor &&
                    linkEligible(_e[j].rxp, _e[j].range)) { shared = true; break; }
            if (shared) { sum += linkQuality(_e[i].rxp, _e[i].range); c++; }
        }
        return c ? sum / c : -1e9f;
    }

private:
    struct Entry { uint16_t anchor; uint16_t tag; float rxp; float range; };
    Entry   _e[TR_MAX_ENTRIES];
    uint8_t _n = 0;
};

#endif // TAG_REGISTRY_H
