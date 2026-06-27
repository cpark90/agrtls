/*
 * rf_config_dw3000.h
 *
 * DW3000 계열 변종의 RF 설정 + 핀. CHIP_DW3000 빌드에서만 사용.
 *
 * 주의: DW3000은 DW1000과 라이브러리 API가 다르다.
 *   - 110kbps 데이터레이트 없음 (850kbps / 6.8Mbps만).
 *   - 채널 5(6.5GHz)/채널 9(8GHz) 중심. 채널 2 등 DW1000용 설정 그대로 쓰지 말 것.
 *   - 아래는 Makerfabs DW3000 라이브러리(NConcepts 기반) 일반 패턴 기준 placeholder.
 *     실제 라이브러리의 설정 구조체/함수명에 맞춰 조정 필요.
 */

#ifndef RF_CONFIG_DW3000_H
#define RF_CONFIG_DW3000_H

// 실제 DW3000 라이브러리 헤더로 교체할 것 (예: #include <dw3000.h>)
// #include <dw3000.h>

// ---- 핀 (Makerfabs ESP32 UWB DW3000) ----
#define PIN_RST   27
#define PIN_SS    21
#define PIN_IRQ   34
#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23

// ---- RF 모드별 파라미터 ----
// DW3000은 110kbps가 없으므로 ACCURACY도 850kbps 기반으로 정의한다.
#if defined(RFMODE_ACCURACY)
  #define DW3000_CHANNEL        5
  #define DW3000_DATARATE       DWT_BR_850K
  #define DW3000_PREAMBLE_LEN   DWT_PLEN_1024
  #define DW3000_PRF            DWT_PRF_64M
#elif defined(RFMODE_FAST)
  #define DW3000_CHANNEL        5
  #define DW3000_DATARATE       DWT_BR_6M8
  #define DW3000_PREAMBLE_LEN   DWT_PLEN_128
  #define DW3000_PRF            DWT_PRF_64M
#elif defined(RFMODE_LOWPOWER)
  #define DW3000_CHANNEL        9
  #define DW3000_DATARATE       DWT_BR_850K
  #define DW3000_PREAMBLE_LEN   DWT_PLEN_1024
  #define DW3000_PRF            DWT_PRF_64M
#else
  #error "RF mode not defined. Define one of: RFMODE_ACCURACY / RFMODE_FAST / RFMODE_LOWPOWER"
#endif

// ---- 안테나 딜레이 (DW3000 기본값은 DW1000과 다름) ----
#define DW3000_ANTENNA_DELAY_DEFAULT  16385
#define DW3000_ANTENNA_DELAY          DW3000_ANTENNA_DELAY_DEFAULT

#if DW3000_ANTENNA_DELAY == DW3000_ANTENNA_DELAY_DEFAULT
  #warning "DW3000_ANTENNA_DELAY is the uncalibrated default. Run the calibration procedure."
#endif

// applyRfConfigDW3000()는 실제 DW3000 라이브러리 초기화 흐름에 맞춰 구현할 것.
// (dwt_configure(&config) 형태가 일반적)

#endif // RF_CONFIG_DW3000_H
