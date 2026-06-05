#include <SPI.h>
#include <DW1000Ranging.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define TAG_ADD "00:T0"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS  = 4;  // spi select pin

// ===== WiFi 설정 (본인 환경에 맞게 수정) =====
const char* WIFI_SSID = "sberry";
const char* WIFI_PASS = "dlahxpq!";

// 데이터를 받을 PC/서버의 IP와 포트
const char*    SERVER_IP   = "192.168.0.8";
const uint16_t SERVER_PORT = 5005;

WiFiUDP udp;
char packetBuffer[128];

void connectWiFi() {
    Serial.print("WiFi connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
    }
    Serial.print(" connected, IP: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // DW1000 SPI보다 먼저 WiFi 연결
    connectWiFi();

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000.setChannel(DW1000.CHANNEL_1);
    DW1000.setPreambleCode(DW1000.PREAMBLE_CODE_16MHZ_2);
    DW1000.setDataRate(DW1000.TRX_RATE_850KBPS);
    DW1000.setPreambleLength(DW1000.TX_PREAMBLE_LEN_1024);
    DW1000.commitConfiguration();

    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    DW1000Ranging.startAsTag(TAG_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
}

void loop() {
    DW1000Ranging.loop();
}

// UDP로 문자열 한 줄 전송
void sendUDP(const char* msg) {
    udp.beginPacket(SERVER_IP, SERVER_PORT);
    udp.write((const uint8_t*)msg, strlen(msg));
    udp.endPacket();
}

void newRange() {
    uint16_t addr   = DW1000Ranging.getDistantDevice()->getShortAddress();
    float    range  = DW1000Ranging.getDistantDevice()->getRange();
    float    rxpow  = DW1000Ranging.getDistantDevice()->getRXPower();

    snprintf(packetBuffer, sizeof(packetBuffer),
             "RANGE,%04X,%.3f,%.2f\n", addr, range, rxpow);

    sendUDP(packetBuffer);
    Serial.print(packetBuffer);   // 디버깅용, 필요 없으면 삭제
}

void newDevice(DW1000Device *device) {
    snprintf(packetBuffer, sizeof(packetBuffer),
             "NEW,%04X\n", device->getShortAddress());
    sendUDP(packetBuffer);
    Serial.print(packetBuffer);
}

void inactiveDevice(DW1000Device *device) {
    snprintf(packetBuffer, sizeof(packetBuffer),
             "DEL,%04X\n", device->getShortAddress());
    sendUDP(packetBuffer);
    Serial.print(packetBuffer);
}
