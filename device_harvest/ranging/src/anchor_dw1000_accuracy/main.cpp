/*
 * 변종: anchor_dw1000_accuracy
 * 앵커 / DW1000 / ACCURACY 모드. 가장 기본형 앵커.
 */
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

// 앵커 #0: byte[0]=0x00 → short addr=0 → "A0"
#define ANCHOR_ADDR "00:00:5B:D5:A9:9A:E2:9C"

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    char devId[8];
    shortAddrToId(d->getShortAddress(), devId, sizeof(devId));
    logRange(devId, d->getRange(), d->getRXPower());
}
void newDevice(DW1000Device* d){ Serial.print("# +dev "); Serial.println(d->getShortAddress(),HEX); }
void inactiveDevice(DW1000Device* d){ Serial.print("# -dev "); Serial.println(d->getShortAddress(),HEX); }

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# anchor_dw1000_accuracy");
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.startAsAnchor(ANCHOR_ADDR, RF_MODE, false);
    applyRfConfigDW1000();
}
void loop(){ DW1000Ranging.loop(); }
