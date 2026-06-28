/*
 * logging.h  (칩 독립)
 *
 * 모든 변종이 공유하는 시리얼 출력 포맷. DW1000/DW3000 무관.
 * 후단(외부 EKF/IMU 융합)의 파서가 이 포맷에 의존하므로 필드 순서를 바꾸지 말 것.
 * 새 필드는 항상 뒤에 append. (CLAUDE.md 규칙)
 *
 * 포맷(CSV): deviceId,range_m,rxPower_dBm,timestamp_ms,nlosFlag
 *
 * deviceId는 수신 측 distant device의 short address에서 파생.
 *   shortAddrToId() 참고: 앵커→"A{n}", 태그→"T{n}"
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <stdint.h>

#ifndef NLOS_RXPOWER_THRESHOLD_DBM
#define NLOS_RXPOWER_THRESHOLD_DBM   (-82.0f)
#endif

// short address → human-readable ID
// 앵커(0x00..0x3F): "A0".."A63"
// 태그 (0x80..0xBF): "T0".."T63"
// 규칙: docs/VARIANTS.md "Short Address Assignment Rule" 참고
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
