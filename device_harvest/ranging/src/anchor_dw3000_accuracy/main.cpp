/*
 * 변종: anchor_dw3000_accuracy
 * 앵커 / DW3000 / ACCURACY 모드.
 *
 * 주의: DW3000은 DW1000과 라이브러리 API가 완전히 다르다.
 *   Makerfabs DW3000 라이브러리(NConcepts 기반)는 보통 range_tx/range_rx
 *   예제처럼 dwt_* 저수준 API 또는 자체 래퍼를 쓴다.
 *   아래는 구조만 잡은 스켈레톤 — 실제 라이브러리 함수로 채워야 함.
 *   (DW1000의 DW1000Ranging 같은 고수준 콜백 API가 없을 수 있음)
 */
#include <Arduino.h>
#include <SPI.h>
#include "rf_config_dw3000.h"
#include "logging.h"

// TODO: 실제 DW3000 라이브러리 헤더 include
// #include "dw3000.h"

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# anchor_dw3000_accuracy");

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);

    // TODO: DW3000 초기화 (예시 흐름)
    //   spiBegin(PIN_IRQ, PIN_RST);
    //   spiSelect(PIN_SS);
    //   if (dwt_initialise(...) == DWT_ERROR) { ... }
    //   dwt_configure(&dw3000_config);  // rf_config_dw3000.h의 DW3000_* 값 사용
    //   dwt_setantennadelay(DW3000_ANTENNA_DELAY);
    //
    // 이후 DS-TWR responder 루틴 (POLL 수신 → RESP 송신 → FINAL 수신 → 거리계산)
    // 계산된 거리/RX Power를 logRange("ANCHOR_01", range, rxp) 로 출력.
}

void loop() {
    // TODO: DS-TWR responder 상태머신
    // 거리 산출 시:
    //   logRange("ANCHOR_01", range_m, rxPower_dBm);
}
