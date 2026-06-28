/*
 * 변종: tag_dw1000_responder
 *
 * 이동 태그. CORE mesh-TDMA 에서 태그는 mf-DW1000 의 responder(startAsAnchor) 역할 —
 * 앵커(initiator)의 폴에 응답만 한다. mesh/스케줄/슬롯 없음(가장 단순).
 *   (역할 반전 근거: docs/DESIGN_FLOW_mesh_tdma.md Pivot 4~5)
 *
 * F-a 검증용 짝: anchor_dw1000_accuracy_meshagent(initiator)가 이 노드를 폴한다.
 */
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

// 태그 번호 (보드마다 다르게). byte0 = 0x80 + TAG_ID → short 0x008N → "T{TAG_ID}".
// 다중 태그 검증 시 보드별로 빌드 오버라이드:
//   PLATFORMIO_BUILD_FLAGS="-DTAG_ID=1" pio run -e tag_dw1000_responder -t upload --upload-port /dev/ttyUSBx
#ifndef TAG_ID
#define TAG_ID 0
#endif

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    char devId[8];
    shortAddrToId(d->getShortAddress(), devId, sizeof(devId));  // 폴한 앵커 → "A.."
    logRange(devId, d->getRange(), d->getRXPower());
}
void newDevice(DW1000Device* d)      { Serial.print("# +dev "); Serial.println(d->getShortAddress(), HEX); }
void inactiveDevice(DW1000Device* d) { Serial.print("# -dev "); Serial.println(d->getShortAddress(), HEX); }

void setup() {
    Serial.begin(115200); delay(200);
    char selfAddr[24];
    snprintf(selfAddr, sizeof(selfAddr), "%02X:00:22:EA:82:60:3B:9C", 0x80 + (TAG_ID));
    Serial.print("# tag_dw1000_responder T"); Serial.println(TAG_ID);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.startAsAnchor(selfAddr, RF_MODE, false);  // responder 역할
    applyRfConfigDW1000();
}
void loop() { DW1000Ranging.loop(); }
