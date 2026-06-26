#include <SPI.h>
#include <DW1000Ranging.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define ANCHOR_ADD "00:A0"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS  = 4;  // spi select pin

WiFiUDP udp;
char packetBuffer[128];

void setup() {
    Serial.begin(115200);
    delay(1000);

    // DW1000 SPI보다 먼저 WiFi 연결

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

	DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);

	DW1000Ranging.attachNewRange(newRange);
	DW1000Ranging.attachNewDevice(newDevice);
	DW1000Ranging.attachInactiveDevice(inactiveDevice);

	// 모드 + 네트워크 설정은 startAsAnchor 한 곳에서
	DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_ACCURACY, false);

	// 채널만 모드 적용 후 별도로 변경 (모드 프리셋에 채널이 없으므로 안전)
	DW1000.setChannel(DW1000.CHANNEL_2);
	DW1000.setPreambleCode(DW1000.PREAMBLE_CODE_64MHZ_9);
	DW1000.commitConfiguration();
}

void loop() {
    DW1000Ranging.loop();
}

void newRange() {
    uint16_t addr   = DW1000Ranging.getDistantDevice()->getShortAddress();
    float    range  = DW1000Ranging.getDistantDevice()->getRange();
    float    rxpow  = DW1000Ranging.getDistantDevice()->getRXPower();

    snprintf(packetBuffer, sizeof(packetBuffer),
             "RANGE,%04X,%.3f,%.2f\n", addr, range, rxpow);

    Serial.print(packetBuffer);   // 디버깅용, 필요 없으면 삭제
}

void newDevice(DW1000Device *device) {
    snprintf(packetBuffer, sizeof(packetBuffer),
             "NEW,%04X\n", device->getShortAddress());
    Serial.print(packetBuffer);
}

void inactiveDevice(DW1000Device *device) {
    snprintf(packetBuffer, sizeof(packetBuffer),
             "DEL,%04X\n", device->getShortAddress());
    Serial.print(packetBuffer);
}
