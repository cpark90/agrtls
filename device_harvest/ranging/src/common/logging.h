/*
 * logging.h  (칩 독립)
 *
 * 모든 변종이 공유하는 시리얼 출력 포맷. DW1000/DW3000 무관.
 * 후단(외부 EKF/IMU 융합)의 파서가 이 포맷에 의존하므로 필드 순서를 바꾸지 말 것.
 * 새 필드는 항상 뒤에 append. (CLAUDE.md 규칙)
 *
 * 포맷(CSV): deviceId,range_m,rxPower_dBm,timestamp_ms,nlosFlag
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

#ifndef NLOS_RXPOWER_THRESHOLD_DBM
#define NLOS_RXPOWER_THRESHOLD_DBM   (-82.0f)
#endif

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
