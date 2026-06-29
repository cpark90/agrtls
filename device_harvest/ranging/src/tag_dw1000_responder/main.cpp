/*
 * Variant: tag_dw1000_responder
 *
 * Mobile tag. In CORE mesh-TDMA the tag is the mf-DW1000 responder (startAsResponder) --
 * it only replies to the initiator (anchor) poll. No mesh / schedule / slots (simplest form).
 *   (role-inversion rationale: docs/DESIGN_FLOW_mesh_tdma.md Pivot 4-5)
 *
 * F-a validation pair: anchor_dw1000_accuracy_meshagent (initiator) polls this node.
 */
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

// Tag number (different per board). byte0 = 0x80 + TAG_ID -> short 0x008N -> "T{TAG_ID}".
// For multi-tag validation, override per board at build time:
//   PLATFORMIO_BUILD_FLAGS="-DTAG_ID=1" pio run -e tag_dw1000_responder -t upload --upload-port /dev/ttyUSBx
#ifndef TAG_ID
#define TAG_ID 0
#endif

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    char devId[8];
    shortAddrToId(d->getShortAddress(), devId, sizeof(devId));  // the polling anchor -> "A.."
    logRange(devId, d->getRange(), d->getRXPower());
}

void setup() {
    Serial.begin(115200); delay(200);
    char selfAddr[24];
    snprintf(selfAddr, sizeof(selfAddr), "%02X:00:22:EA:82:60:3B:9C", 0x80 + (TAG_ID));
    Serial.print("# tag_dw1000_responder T"); Serial.println(TAG_ID);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.startAsResponder(selfAddr, RF_MODE, false);  // reply to polls only
    applyRfConfigDW1000();
}
void loop() { DW1000Ranging.loop(); }
