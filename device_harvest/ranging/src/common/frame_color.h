/*
 * frame_color.h  (chip-independent, pure)
 *
 * Deterministic tag→frame coloring for synchronous TDMA (docs/ARCHITECTURE_synchronous_tdma.md §4). Every
 * anchor runs this on the shared TagRegistry and gets the SAME result, so no schedule needs to be
 * exchanged — only the registry.
 *
 *   - conflict = two tags share an EFFECTIVE anchor (TagRegistry::conflict)
 *   - color = frame; conflicting tags get different colors; non-conflicting tags reuse a color
 *     (spatial reuse)
 *   - tags are colored in priority order (meanQuality desc, tie by id) so near/strong tags take the
 *     low frames; a tag that would need a color >= maxFrames, or has no effective anchor, becomes a
 *     CANDIDATE (far/weak: measured only via probe frames, never permanently dropped)
 *
 * Greedy graph coloring (O(tags^2) over conflicts) — fine for the per-neighborhood tag counts here.
 */

#ifndef FRAME_COLOR_H
#define FRAME_COLOR_H

#include <stdint.h>
#include "tag_registry.h"

#ifndef FC_MAX_TAGS
#define FC_MAX_TAGS 24
#endif
#ifndef FC_MAX_FRAMES
#define FC_MAX_FRAMES 10      // upper bound on ACTIVE (localization) frames -> bounds superframe length
#endif
#define FC_CANDIDATE 0xFF      // not assigned to a normal frame
#define FC_NONE      0xFFFF

class FrameColor {
public:
    // Build the coloring from the registry. maxFrames caps the schedule length; tags overflowing it
    // (or with no effective anchor) become candidates.
    void build(const TagRegistry& reg, uint8_t maxFrames) {
        _n = reg.tags(_tag, FC_MAX_TAGS);
        // priority order: meanQuality desc, tie by tagId asc (deterministic on every anchor)
        for (uint8_t i = 0; i < _n; i++) {
            _color[i] = FC_CANDIDATE;
            _q[i] = reg.meanQuality(_tag[i]);
        }
        for (uint8_t a = 0; a < _n; a++)
            for (uint8_t b = (uint8_t)(a + 1); b < _n; b++)
                if (_q[b] > _q[a] || (_q[b] == _q[a] && _tag[b] < _tag[a])) {
                    swapIdx(a, b);
                }

        _numFrames = 0;
        for (uint8_t i = 0; i < _n; i++) {
            if (reg.effectiveAnchorCount(_tag[i]) == 0) continue;   // candidate (no eligible anchor)
            uint8_t c = lowestFreeColor(reg, i, maxFrames);
            if (c == FC_CANDIDATE) continue;                        // overflowed cap -> candidate
            _color[i] = c;
            if (c + 1 > _numFrames) _numFrames = (uint8_t)(c + 1);
        }
    }

    uint8_t numFrames() const { return _numFrames; }

    // Frame color of a tag, or FC_CANDIDATE if it is a candidate.
    uint8_t colorOf(uint16_t tagId) const {
        int i = indexOf(tagId);
        return (i < 0) ? FC_CANDIDATE : _color[i];
    }

    bool isCandidate(uint16_t tagId) const { return colorOf(tagId) == FC_CANDIDATE; }

    // List candidate tags (no normal frame).
    uint8_t candidates(uint16_t* out, uint8_t maxN) const {
        uint8_t c = 0;
        for (uint8_t i = 0; i < _n && c < maxN; i++)
            if (_color[i] == FC_CANDIDATE) out[c++] = _tag[i];
        return c;
    }

    // Which tag should `anchorId` poll in frame `frameIdx`? At most one (an anchor's effective tags
    // are mutually conflicting -> distinct colors). FC_NONE if none.
    uint16_t tagForFrame(const TagRegistry& reg, uint16_t anchorId, uint8_t frameIdx) const {
        uint16_t effA[TR_MAX_TAGS];   // reuse cap
        for (uint8_t i = 0; i < _n; i++) {
            if (_color[i] != frameIdx) continue;
            uint8_t na = reg.effectiveAnchors(_tag[i], effA, TR_MAX_TAGS);
            for (uint8_t k = 0; k < na; k++)
                if (effA[k] == anchorId) return _tag[i];
        }
        return FC_NONE;
    }

private:
    int indexOf(uint16_t tagId) const {
        for (uint8_t i = 0; i < _n; i++) if (_tag[i] == tagId) return (int)i;
        return -1;
    }
    void swapIdx(uint8_t a, uint8_t b) {
        uint16_t t = _tag[a]; _tag[a] = _tag[b]; _tag[b] = t;
        float q = _q[a]; _q[a] = _q[b]; _q[b] = q;
    }
    // lowest color not used by an already-colored conflicting tag; FC_CANDIDATE if it reaches the cap.
    uint8_t lowestFreeColor(const TagRegistry& reg, uint8_t idx, uint8_t maxFrames) const {
        for (uint8_t c = 0; c < maxFrames; c++) {
            bool used = false;
            for (uint8_t j = 0; j < _n; j++)
                if (j != idx && _color[j] == c && reg.conflict(_tag[idx], _tag[j])) { used = true; break; }
            if (!used) return c;
        }
        return FC_CANDIDATE;
    }

    uint16_t _tag[FC_MAX_TAGS];
    uint8_t  _color[FC_MAX_TAGS];
    float    _q[FC_MAX_TAGS];
    uint8_t  _n = 0;
    uint8_t  _numFrames = 0;
};

#endif // FRAME_COLOR_H
