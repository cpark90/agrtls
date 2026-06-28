/*
 * 변종: tag_dw3000_fast_filtered
 * 태그 / DW3000 / FAST 모드 / 소프트웨어 필터링 적용.
 *
 * FAST 모드(6.8Mbps)로 빠른 업데이트 + RangeFilter로 출렁임 완화.
 * DW3000 저수준 API 부분은 실제 라이브러리로 채울 것.
 */
#include <Arduino.h>
#include <SPI.h>
#include "rf_config_dw3000.h"
#include "logging.h"
#include "range_filter.h"

// TODO: 실제 DW3000 라이브러리 헤더 include
// #include "dw3000.h"

RangeFilter rangeFilter(0.3f, 0.5f);  // alpha, outlier threshold(m)

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# tag_dw3000_fast_filtered");

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);

    // TODO: DW3000 초기화 + dwt_configure(&dw3000_config) (FAST 파라미터)
    //       dwt_setantennadelay(DW3000_ANTENNA_DELAY);
}

void loop() {
    // TODO: DS-TWR initiator 상태머신으로 raw 거리 측정
    // float raw = ...;  float rxp = ...;
    // float filtered = rangeFilter.update(raw);
    // char devId[8]; shortAddrToId(anchorShortAddr, devId, sizeof(devId));
    // logRange(devId, filtered, rxp);
}
