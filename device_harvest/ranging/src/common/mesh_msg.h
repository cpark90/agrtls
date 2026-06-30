/*
 * mesh_msg.h  (chip-independent, pure)
 *
 * Window-TDMA mesh message: TAGINFO — an anchor's per-tag measurements shared so every anchor builds
 * the same TagRegistry. SYNC + the byte cursors are in mesh_wire.h (shared); the mesh-TDMA MGM
 * messages (VALUE/GAIN/TAGLIST/AUDIBLE) are in mgm_msg.h (the meshagent variant).
 *
 * Pure: host unit-testable.
 */

#ifndef MESH_MSG_H
#define MESH_MSG_H

#include <stdint.h>
#include "mesh_wire.h"

enum : uint8_t {
    MESH_TAGINFO = 6,   // anchorId(2) count(1) [tagId(2) rxp_cdBm(2) range_cm(2) elig(1)]*
};

#ifndef MESH_MAX_TAGINFO
#define MESH_MAX_TAGINFO 16        // tag measurements per TAGINFO frame
#endif
// Largest window message = TAGINFO: type(1) + anchorId(2) + count(1) + 7 bytes/entry.
#define MESH_MAX_FRAME (4 + 7 * MESH_MAX_TAGINFO)

// --- TAGINFO (registry share): an anchor's per-tag measurements ---
// `eligible` is the OWNER anchor's authoritative link-eligibility decision (computed once, from its
// own smoothed history). Receivers adopt it verbatim instead of recomputing, so every anchor agrees
// on each link's eligibility -> the coloring / slot counts stay consistent across anchors.
struct TagInfoEntry { uint16_t tagId; float rxp_dBm; float range_m; bool eligible; };

namespace mesh_detail {
inline int16_t  toCentiDbm(float dbm)  { return (int16_t)(dbm >= 0 ? dbm * 100 + 0.5f : dbm * 100 - 0.5f); }
inline uint16_t toCm(float m)          { if (m < 0) m = 0; float c = m * 100 + 0.5f; return c > 65535.0f ? 65535 : (uint16_t)c; }
}

inline uint8_t packTagInfo(uint16_t anchorId, const TagInfoEntry* e, uint8_t n, uint8_t* buf) {
    if (n > MESH_MAX_TAGINFO) n = MESH_MAX_TAGINFO;
    uint8_t* p = buf;
    *p++ = MESH_TAGINFO;
    mesh_detail::putU16(p, anchorId);
    *p++ = n;
    for (uint8_t i = 0; i < n; i++) {
        mesh_detail::putU16(p, e[i].tagId);
        mesh_detail::putU16(p, (uint16_t)mesh_detail::toCentiDbm(e[i].rxp_dBm));
        mesh_detail::putU16(p, mesh_detail::toCm(e[i].range_m));
        *p++ = e[i].eligible ? 1 : 0;                           // owner's eligibility decision
    }
    return (uint8_t)(p - buf);
}

inline bool unpackTagInfo(const uint8_t* buf, uint8_t len,
                          uint16_t& anchorId, TagInfoEntry* out, uint8_t maxN, uint8_t& n) {
    if (len < 4 || buf[0] != MESH_TAGINFO) return false;
    const uint8_t* p = buf + 1;
    anchorId = mesh_detail::getU16(p);
    uint8_t count = *p++;
    if ((uint16_t)(4 + 7 * count) > len) return false;          // truncated
    n = (count > maxN) ? maxN : count;
    for (uint8_t i = 0; i < n; i++) {
        out[i].tagId    = mesh_detail::getU16(p);
        out[i].rxp_dBm  = (int16_t)mesh_detail::getU16(p) / 100.0f;
        out[i].range_m  = mesh_detail::getU16(p) / 100.0f;
        out[i].eligible = (*p++ != 0);
    }
    return true;
}

#endif // MESH_MSG_H
