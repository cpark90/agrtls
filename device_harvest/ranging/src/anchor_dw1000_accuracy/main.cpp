/*
 * Variant: anchor_dw1000_accuracy
 * Standard anchor / DW1000 / ACCURACY mode.
 *
 * Native mf-DW1000 roles (no mesh, no scheduling): the anchor is the RESPONDER (startAsAnchor)
 * and pairs with tag_dw1000_accuracy (the initiator that broadcasts POLL). Uses the library as-is
 * (scheduledMode stays off). Up to ~4 anchors per tag (broadcast RANGE frame limit).
 *
 * Note: roles are the OPPOSITE of the distributed TDMA variants, where the physical anchor is the initiator
 * (anchor_dw1000_accuracy_meshagent). See docs/VARIANTS.md.
 */
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

// Anchor number (different per board). byte0 = ANCHOR_ID -> short 0x00NN -> "A{ANCHOR_ID}".
//   PLATFORMIO_BUILD_FLAGS="-DANCHOR_ID=1" pio run -e anchor_dw1000_accuracy -t upload --upload-port /dev/ttyUSBx
#ifndef ANCHOR_ID
#define ANCHOR_ID 0
#endif

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    char devId[8];
    shortAddrToId(d->getShortAddress(), devId, sizeof(devId));  // the polling tag -> "T.."
    logRange(devId, d->getRange(), d->getRXPower());
}

void setup() {
    Serial.begin(115200); delay(200);
    char selfAddr[24];
    snprintf(selfAddr, sizeof(selfAddr), "%02X:00:5B:D5:A9:9A:E2:9C", (ANCHOR_ID));
    Serial.print("# anchor_dw1000_accuracy A"); Serial.println(ANCHOR_ID);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.startAsAnchor(selfAddr, RF_MODE, false);  // responder
    applyRfConfigDW1000();
}
void loop() { DW1000Ranging.loop(); }
