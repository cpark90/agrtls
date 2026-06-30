/*
 * mesh_wire.h  (chip-independent, pure)
 *
 * Shared wire utilities for the ESP-NOW mesh control plane: little-endian byte cursors, the frame
 * type accessor, and the SYNC (slot-phase gossip) message used by BOTH TDMA models. Per-model message
 * sets live elsewhere: mesh_msg.h (window-TDMA: TAGINFO), mgm_msg.h (mesh-TDMA: VALUE/GAIN/...).
 *
 * Pure: no radio/Arduino, host unit-testable.
 */

#ifndef MESH_WIRE_H
#define MESH_WIRE_H

#include <stdint.h>
#include <stddef.h>

enum : uint8_t { MESH_SYNC = 5 };   // slot-phase gossip: agentId(2) phaseMs(4)

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

// --- SYNC (slot-phase gossip) — used by both window and mesh TDMA ---
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

#endif // MESH_WIRE_H
