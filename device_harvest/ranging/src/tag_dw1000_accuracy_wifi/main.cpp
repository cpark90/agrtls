/*
 * 변종: tag_dw1000_accuracy_wifi
 * 태그 / DW1000 / ACCURACY 모드 / WiFi로 거리 데이터 원격 전송.
 * 거리/RX Power를 UDP로 서버에 송출 (후단 EKF 시스템 입력용).
 */
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <DW1000Ranging.h>
#include "rf_config_dw1000.h"
#include "logging.h"

// TODO: 실제 환경 값으로 교체
static const char* WIFI_SSID = "YOUR_SSID";
static const char* WIFI_PASS = "YOUR_PASS";
static const char* SERVER_IP = "192.168.0.100";
static const uint16_t SERVER_PORT = 9000;

WiFiUDP udp;

// 태그 #1: byte[0]=0x81 → short addr=129 → "T1"
#define TAG_ADDR "81:00:22:EA:82:60:3B:9C"

void sendUdp(const char* devId, float range_m, float rxp_dBm) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "%s,%.3f,%.2f,%lu",
                     devId, range_m, rxp_dBm, (unsigned long)millis());
    udp.beginPacket(SERVER_IP, SERVER_PORT);
    udp.write((const uint8_t*)buf, n);
    udp.endPacket();
}

void newRange() {
    DW1000Device* d = DW1000Ranging.getDistantDevice();
    float r = d->getRange();
    float p = d->getRXPower();
    char devId[8];
    shortAddrToId(d->getShortAddress(), devId, sizeof(devId));
    logRange(devId, r, p);
    if (WiFi.status() == WL_CONNECTED) sendUdp(devId, r, p);
}
void newDevice(DW1000Device* d){ Serial.print("# +dev "); Serial.println(d->getShortAddress(),HEX); }
void inactiveDevice(DW1000Device* d){ Serial.print("# -dev "); Serial.println(d->getShortAddress(),HEX); }

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# tag_dw1000_accuracy_wifi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // 비차단: 연결 안 돼도 레인징은 계속. loop에서 상태만 확인.

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.startAsTag(TAG_ADDR, RF_MODE, false);
    applyRfConfigDW1000();
}
void loop(){ DW1000Ranging.loop(); }
