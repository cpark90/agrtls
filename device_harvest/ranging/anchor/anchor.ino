#include <SPI.h>
#include "DW1000Ranging.h"

#define ANCHOR_ADD "00:A0"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 4;   // spi select pin

void setup() {
    Serial.begin(115200);
    delay(1000);
    //init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

	// 1. DW1000 초기화 및 핀 설정
	DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);


	// 2. [핵심] 채널 1번 및 드라이버 파라미터 강제 설정
	// 기본 탑재된 기기 설정(Configuration)을 초기화합니다.
	DW1000.reinit();

	// 드라이버 기본 설정값을 불러옵니다.
	DW1000.defaults();

	// 채널을 5번에서 1번으로 변경 (채널 1은 중심 주파수 3.5 GHz 대역 사용)
	DW1000.setChannel(CHANNEL_1);

	// 채널 1번에 사용할 수 있는 프리앰블 코드(Preamble Code) 지정
	// 채널 1번은 규격상 1번 또는 2번 코드를 사용할 수 있습니다. (여기서는 2번 지정)
	DW1000.setPreambleCode(PREAMBLE_CODE_2);

	// 무선 데이터 전송 속도 설정 (기본 6.8Mbps -> 850kbps로 낮추면 수신 감도와 도달 거리가 비약적으로 상승)
	DW1000.setDataRate(TRX_RATE_850KBPS);

	// 프리앰블 길이 설정 (장거리 및 데이터 안정성을 위해 1024 지정)
	DW1000.setPreambleLength(LEN_1024);

	// 펄스 반복 주파수(Pulse Repetition Frequency) 설정 (16MHz 또는 64MHz)
	DW1000.setPulseFrequency(TX_PULSE_FREQ_16MHZ);

	// PAC(Preamble Acquisition Chunk) 크기 설정 (프리앰블 길이에 맞춰 최적화)
	DW1000.setPacSize(PAC_SIZE_16);

	// 변경된 설정값을 DW1000 시스템 내부에 최종 적용 및 칩셋 작동 시작
	DW1000.commitConfiguration();



    //define the sketch as anchor. It will be great to dynamically change the type of module
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    //Enable the filter to smooth the distance
    //DW1000Ranging.useRangeFilter(true);

    DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
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
