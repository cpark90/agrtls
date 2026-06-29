/*
 * mesh_msg.h  (chip-independent, pure)
 *
 * Wire (de)serialization for the L2/L3 coordination messages exchanged between anchors over the
 * mesh control plane (ESP-NOW broadcast in F-b). Little-endian, fixed compact layout.
 *
 * Frame = [type(1)] [payload...]
 *   VALUE   : round(2) agentId(2) slot(1) leaseExpiry(4)          = 10 B
 *   GAIN    : round(2) agentId(2) gain(2) propSlot(1)             =  8 B
 *   TAGLIST : agentId(2) count(1) ids[count](2 each)              <= 27 B (12 ids)
 *   AUDIBLE : agentId(2) count(1) ids[count](2 each)              <= 27 B (12 ids)
 *
 * Pure: no radio/Arduino, so it is host unit-testable.
 */

#ifndef MESH_MSG_H
#define MESH_MSG_H

#include <stdint.h>
#include <stddef.h>
#include "mgm_agent.h"   // ValueMsg, GainMsg

enum : uint8_t {
    MESH_VALUE   = 1,
    MESH_GAIN    = 2,
    MESH_TAGLIST = 3,
    MESH_AUDIBLE = 4,
    MESH_SYNC    = 5,   // slot-phase gossip: agentId(2) phaseMs(4)
    MESH_TAGINFO = 6,   // registry share: anchorId(2) count(1) [tagId(2) rxp_cdBm(2) range_cm(2)]*
};

#ifndef MESH_MAX_IDS
#define MESH_MAX_IDS 12
#endif
#ifndef MESH_MAX_TAGINFO
#define MESH_MAX_TAGINFO 16        // tag measurements per TAGINFO frame
#endif
// Largest message = TAGINFO: type(1) + anchorId(2) + count(1) + 7 bytes/entry.
#define MESH_MAX_FRAME (4 + 7 * MESH_MAX_TAGINFO)

// --- little-endian cursor helpers ---
namespace mesh_detail {
inline void     putU16(uint8_t*& p, uint16_t v) { *p++ = (uint8_t)(v & 0xFF); *p++ = (uint8_t)(v >> 8); }
inline uint16_t getU16(const uint8_t*& p)        { uint16_t v = (uint16_t)p[0] | ((uint16_t)p[1] << 8); p += 2; return v; }
inline void     putU32(uint8_t*& p, uint32_t v) { for (int i = 0; i < 4; i++) { *p++ = (uint8_t)(v & 0xFF); v >>= 8; } }
inline uint32_t getU32(const uint8_t*& p)        {
    uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    p += 4; return v;
}
}

// frame type, or 0 if empty
inline uint8_t meshMsgType(const uint8_t* buf, uint8_t len) { return (len >= 1) ? buf[0] : 0; }

// --- VALUE ---
inline uint8_t packValue(const ValueMsg& m, uint8_t* buf) {
    uint8_t* p = buf;
    *p++ = MESH_VALUE;
    mesh_detail::putU16(p, m.round);
    mesh_detail::putU16(p, m.agentId);
    *p++ = m.slot;
    mesh_detail::putU32(p, m.leaseExpiry);
    return (uint8_t)(p - buf);
}
inline bool unpackValue(const uint8_t* buf, uint8_t len, ValueMsg& m) {
    if (len < 10 || buf[0] != MESH_VALUE) return false;
    const uint8_t* p = buf + 1;
    m.round       = mesh_detail::getU16(p);
    m.agentId     = mesh_detail::getU16(p);
    m.slot        = *p++;
    m.leaseExpiry = mesh_detail::getU32(p);
    return true;
}

// --- GAIN ---
inline uint8_t packGain(const GainMsg& m, uint8_t* buf) {
    uint8_t* p = buf;
    *p++ = MESH_GAIN;
    mesh_detail::putU16(p, m.round);
    mesh_detail::putU16(p, m.agentId);
    mesh_detail::putU16(p, (uint16_t)m.gain);
    *p++ = m.propSlot;
    return (uint8_t)(p - buf);
}
inline bool unpackGain(const uint8_t* buf, uint8_t len, GainMsg& m) {
    if (len < 8 || buf[0] != MESH_GAIN) return false;
    const uint8_t* p = buf + 1;
    m.round    = mesh_detail::getU16(p);
    m.agentId  = mesh_detail::getU16(p);
    m.gain     = (int16_t)mesh_detail::getU16(p);
    m.propSlot = *p++;
    return true;
}

// --- TAGLIST / AUDIBLE (same id-list layout, different type byte) ---
inline uint8_t packIdList(uint8_t type, uint16_t agentId, const uint16_t* ids, uint8_t n, uint8_t* buf) {
    if (n > MESH_MAX_IDS) n = MESH_MAX_IDS;
    uint8_t* p = buf;
    *p++ = type;
    mesh_detail::putU16(p, agentId);
    *p++ = n;
    for (uint8_t i = 0; i < n; i++) mesh_detail::putU16(p, ids[i]);
    return (uint8_t)(p - buf);
}
// --- SYNC (slot-phase gossip) ---
inline uint8_t packSync(uint16_t agentId, uint32_t phaseMs, uint8_t* buf) {
    uint8_t* p = buf;
    *p++ = MESH_SYNC;
    mesh_detail::putU16(p, agentId);
    mesh_detail::putU32(p, phaseMs);
    return (uint8_t)(p - buf);
}
inline bool unpackSync(const uint8_t* buf, uint8_t len, uint16_t& agentId, uint32_t& phaseMs) {
    if (len < 7 || buf[0] != MESH_SYNC) return false;
    const uint8_t* p = buf + 1;
    agentId = mesh_detail::getU16(p);
    phaseMs = mesh_detail::getU32(p);
    return true;
}

inline bool unpackIdList(const uint8_t* buf, uint8_t len,
                         uint16_t& agentId, uint16_t* ids, uint8_t maxN, uint8_t& n) {
    if (len < 4) return false;
    const uint8_t* p = buf + 1;
    agentId = mesh_detail::getU16(p);
    uint8_t count = *p++;
    if ((uint8_t)(4 + 2 * count) > len) return false;   // truncated
    n = (count > maxN) ? maxN : count;
    for (uint8_t i = 0; i < n; i++) ids[i] = mesh_detail::getU16(p);
    return true;
}

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
