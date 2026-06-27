/*
 * 변종: tag_dw1000_accuracy
 * 태그 / DW1000 / ACCURACY 모드. 가장 기본형 태그.
 */
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

#define TAG_ADDR "7D:00:22:EA:82:60:3B:9C"

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    if (d == nullptr) return;
    logRange("ANCHOR_01", d->getRange(), d->getRXPower());
}
void newDevice(DW1000Device* d){ Serial.print("# +dev "); Serial.println(d->getShortAddress(),HEX); }
void inactiveDevice(DW1000Device* d){ Serial.print("# -dev "); Serial.println(d->getShortAddress(),HEX); }

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# tag_dw1000_accuracy");
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.startAsTag(TAG_ADDR, RF_MODE, false);
    applyRfConfigDW1000();
}
void loop(){ DW1000Ranging.loop(); }
