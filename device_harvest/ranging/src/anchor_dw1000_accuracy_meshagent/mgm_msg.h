/*
 * mgm_msg.h  (meshagent-only, pure)
 *
 * Distributed TDMA (MGM) coordination messages exchanged between anchors over the ESP-NOW control plane:
 *   VALUE   : round(2) agentId(2) slot(1) leaseExpiry(4)   = 10 B
 *   GAIN    : round(2) agentId(2) gain(2) propSlot(1)       =  8 B
 *   TAGLIST : agentId(2) count(1) ids[count](2 each)        <= 27 B
 *   AUDIBLE : agentId(2) count(1) ids[count](2 each)        <= 27 B
 *
 * SYNC + the byte cursors come from mesh_wire.h (shared). Only the anchor_dw1000_accuracy_meshagent
 * variant uses this. Pure: host unit-testable.
 */

#ifndef MGM_MSG_H
#define MGM_MSG_H

#include <stdint.h>
#include "mesh_wire.h"
#include "mgm_agent.h"   // ValueMsg, GainMsg

enum : uint8_t {
    MESH_VALUE   = 1,
    MESH_GAIN    = 2,
    MESH_TAGLIST = 3,
    MESH_AUDIBLE = 4,
};

#ifndef MESH_MAX_IDS
#define MESH_MAX_IDS 12
#endif

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

#endif // MGM_MSG_H
