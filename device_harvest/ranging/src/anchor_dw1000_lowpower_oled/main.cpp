/*
 * 변종: anchor_dw1000_lowpower_oled
 * 앵커 / DW1000 / LOWPOWER 모드 / OLED 디스플레이 출력.
 * SSD1306 1.3" 128x64에 최근 거리값을 표시.
 */
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

// 앵커 #1: byte[0]=0x01 → short addr=1 → "A1"
#define ANCHOR_ADDR "01:00:5B:D5:A9:9A:E2:9C"

static float lastRange = 0.0f;
static float lastRxp   = 0.0f;

void drawOled() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ANCHOR (LOWPOWER)");
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print(lastRange, 2);
    display.println(" m");
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("RXP ");
    display.print(lastRxp, 1);
    display.println(" dBm");
    display.display();
}

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    lastRange = d->getRange();
    lastRxp   = d->getRXPower();
    char devId[8];
    shortAddrToId(d->getShortAddress(), devId, sizeof(devId));
    logRange(devId, lastRange, lastRxp);
    drawOled();
}
void newDevice(DW1000Device* d){ Serial.print("# +dev "); Serial.println(d->getShortAddress(),HEX); }
void inactiveDevice(DW1000Device* d){ Serial.print("# -dev "); Serial.println(d->getShortAddress(),HEX); }

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# anchor_dw1000_lowpower_oled");

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("# OLED init failed");
    } else {
        display.clearDisplay(); display.display();
    }

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.startAsAnchor(ANCHOR_ADDR, RF_MODE, false);
    applyRfConfigDW1000();
}
void loop(){ DW1000Ranging.loop(); }
