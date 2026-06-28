/*
 * rf_config_dw3000.h
 *
 * RF config + pins for DW3000-family variants. Used only in CHIP_DW3000 builds.
 *
 * Note: DW3000's library API differs from DW1000.
 *   - No 110kbps data rate (only 850kbps / 6.8Mbps).
 *   - Centered on channel 5 (6.5GHz) / channel 9 (8GHz). Do not reuse DW1000 settings like channel 2.
 *   - The values below are placeholders following the typical Makerfabs DW3000 library (NConcepts)
 *     pattern. Adjust to the real library's config struct / function names.
 */

#ifndef RF_CONFIG_DW3000_H
#define RF_CONFIG_DW3000_H

// Replace with the real DW3000 library header (e.g. #include <dw3000.h>)
// #include <dw3000.h>

// ---- pins (Makerfabs ESP32 UWB DW3000) ----
#define PIN_RST   27
#define PIN_SS    21
#define PIN_IRQ   34
#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23

// ---- per-mode RF parameters ----
// DW3000 has no 110kbps, so ACCURACY is also defined on top of 850kbps.
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

// ---- antenna delay (DW3000 default differs from DW1000) ----
#define DW3000_ANTENNA_DELAY_DEFAULT  16385
#define DW3000_ANTENNA_DELAY          DW3000_ANTENNA_DELAY_DEFAULT

#if DW3000_ANTENNA_DELAY == DW3000_ANTENNA_DELAY_DEFAULT
  #warning "DW3000_ANTENNA_DELAY is the uncalibrated default. Run the calibration procedure."
#endif

// Implement applyRfConfigDW3000() to match the real DW3000 library init flow.
// (typically of the form dwt_configure(&config))

#endif // RF_CONFIG_DW3000_H
