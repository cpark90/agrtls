/*
 * Variant: tag_dw1000_accuracy
 * Standard tag / DW1000 / ACCURACY mode.
 *
 * Native mf-DW1000 roles (no mesh, no scheduling): the tag is the INITIATOR (startAsTag) that
 * broadcasts POLL to the anchors and obtains its ranges. Pairs with anchor_dw1000_accuracy. Uses the
 * library as-is. Broadcast TWR -> up to ~4 anchors (RANGE frame limit).
 *
 * Note: roles are the OPPOSITE of the mesh-TDMA variants (tag_dw1000_responder is a responder).
 * See docs/VARIANTS.md.
 */
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

// Tag number (different per board). byte0 = 0x80 + TAG_ID -> short 0x008N -> "T{TAG_ID}".
//   PLATFORMIO_BUILD_FLAGS="-DTAG_ID=1" pio run -e tag_dw1000_accuracy -t upload --upload-port /dev/ttyUSBx
#ifndef TAG_ID
#define TAG_ID 0
#endif

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    char devId[8];
    shortAddrToId(d->getShortAddress(), devId, sizeof(devId));  // the ranged anchor -> "A.."
    logRange(devId, d->getRange(), d->getRXPower());
}
void newDevice(DW1000Device* d)      { Serial.print("# +dev "); Serial.println(d->getShortAddress(), HEX); }
void inactiveDevice(DW1000Device* d) { Serial.print("# -dev "); Serial.println(d->getShortAddress(), HEX); }

void setup() {
    Serial.begin(115200); delay(200);
    char selfAddr[24];
    snprintf(selfAddr, sizeof(selfAddr), "%02X:00:22:EA:82:60:3B:9C", 0x80 + (TAG_ID));
    Serial.print("# tag_dw1000_accuracy T"); Serial.println(TAG_ID);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.startAsTag(selfAddr, RF_MODE, false);  // initiator (broadcasts POLL)
    applyRfConfigDW1000();
}
void loop() { DW1000Ranging.loop(); }
