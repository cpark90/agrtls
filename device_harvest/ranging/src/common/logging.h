/*
 * logging.h  (chip-independent)
 *
 * Serial output format shared by all variants. DW1000/DW3000-agnostic.
 * The downstream parser (external EKF/IMU fusion) depends on this format, so do not change the
 * field order. Always append new fields at the end. (CLAUDE.md rule)
 *
 * Format (CSV): deviceId,range_m,rxPower_dBm,timestamp_ms,nlosFlag
 *
 * deviceId is derived from the distant device's short address.
 *   see shortAddrToId(): anchor -> "A{n}", tag -> "T{n}"
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <stdint.h>

#ifndef NLOS_RXPOWER_THRESHOLD_DBM
#define NLOS_RXPOWER_THRESHOLD_DBM   (-82.0f)
#endif

// short address -> human-readable ID
// anchor (0x00..0x3F): "A0".."A63"
// tag    (0x80..0xBF): "T0".."T63"
// rule: see docs/VARIANTS.md "Short Address Assignment Rule"
inline void shortAddrToId(uint16_t addr, char* buf, size_t len) {
    uint8_t lo = (uint8_t)(addr & 0xFF);
    if (lo < 0x40)
        snprintf(buf, len, "A%u", (unsigned)lo);
    else if (lo >= 0x80 && lo < 0xC0)
        snprintf(buf, len, "T%u", (unsigned)(lo - 0x80));
    else
        snprintf(buf, len, "X%04X", (unsigned)addr);
}

inline void logRange(const char* deviceId, float range_m, float rxPower_dBm) {
    bool nlosSuspect = (rxPower_dBm < NLOS_RXPOWER_THRESHOLD_DBM);
    Serial.print(deviceId);
    Serial.print(',');
    Serial.print(range_m, 3);
    Serial.print(',');
    Serial.print(rxPower_dBm, 2);
    Serial.print(',');
    Serial.print(millis());
    Serial.print(',');
    Serial.println(nlosSuspect ? 1 : 0);
}

#endif // LOGGING_H
