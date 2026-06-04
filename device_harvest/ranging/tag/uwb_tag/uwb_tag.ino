#include <SPI.h>
#include "DW1000Ranging.h"

#define TAG_ADD "00:T0"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
const uint8_t		PIN_RST = 27; // reset pin
const uint8_t		PIN_IRQ = 34; // irq pin
const uint8_t		PIN_SS = 4;   // spi select pin

void setup() {
    Serial.begin(115200);
    delay(1000);
    //init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
    //define the sketch as anchor. It will be great to dynamically change the type of module
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    //Enable the filter to smooth the distance
    //DW1000Ranging.useRangeFilter(true);

    //we start the module as a tag
    DW1000Ranging.startAsTag(TAG_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
}

void loop() {
    DW1000Ranging.loop();
}

void newRange() {
    Serial.print("RANGE,");
    Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
    Serial.print(",");
    Serial.print(DW1000Ranging.getDistantDevice()->getRange(), 3);
    Serial.print(",");
    Serial.println(DW1000Ranging.getDistantDevice()->getRXPower());
}

void newDevice(DW1000Device *device) {
    Serial.print("NEW,");
    Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device *device) {
    Serial.print("DEL,");
    Serial.println(device->getShortAddress(), HEX);
}
