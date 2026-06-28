/*
 * rf_config_dw1000.h
 *
 * RF config + pins for DW1000-family variants. Used only in CHIP_DW1000 builds.
 * Select the mode via the RFMODE_ACCURACY / RFMODE_LOWPOWER / RFMODE_FAST macro.
 *
 * The 4 parameters (channel/code/data-rate/preamble) must match between anchor and tag to talk.
 */

#ifndef RF_CONFIG_DW1000_H
#define RF_CONFIG_DW1000_H

#include <DW1000.h>

// ---- pins (Makerfabs ESP32 UWB Pro) ----
#define PIN_RST   27
#define PIN_SS    4
#define PIN_IRQ   34
#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23

// OLED I2C (for display variants)
#define PIN_I2C_SDA  4
#define PIN_I2C_SCL  5

#define RFMODE_ACCURACY

// ---- per-mode RF parameters ----
#if defined(RFMODE_ACCURACY)
  #define RF_DATA_RATE        DW1000.TRX_RATE_110KBPS
  #define RF_PREAMBLE_LENGTH  DW1000.TX_PREAMBLE_LEN_2048
  #define RF_MODE             DW1000.MODE_LONGDATA_RANGE_ACCURACY
  #define RF_PREAMBLE_CODE    DW1000.PREAMBLE_CODE_64MHZ_9   // 64MHz PRF
#elif defined(RFMODE_LOWPOWER)
  #define RF_DATA_RATE        DW1000.TRX_RATE_110KBPS
  #define RF_PREAMBLE_LENGTH  DW1000.TX_PREAMBLE_LEN_2048
  #define RF_MODE             DW1000.MODE_LONGDATA_RANGE_LOWPOWER
  #define RF_PREAMBLE_CODE    DW1000.PREAMBLE_CODE_16MHZ_4   // 16MHz PRF
#elif defined(RFMODE_FAST)
  #define RF_DATA_RATE        DW1000.TRX_RATE_6800KBPS
  #define RF_PREAMBLE_LENGTH  DW1000.TX_PREAMBLE_LEN_128
  #define RF_MODE             DW1000.MODE_SHORTDATA_FAST_ACCURACY
  #define RF_PREAMBLE_CODE    DW1000.PREAMBLE_CODE_64MHZ_9
#else
  #error "RF mode not defined. Define one of: RFMODE_ACCURACY / RFMODE_LOWPOWER / RFMODE_FAST"
#endif

#define RF_CHANNEL            DW1000.CHANNEL_2

// ---- antenna delay ----
#define RF_ANTENNA_DELAY_DEFAULT  16384
// TODO: replace with a value measured via the calibration/ procedure.
#define RF_ANTENNA_DELAY          RF_ANTENNA_DELAY_DEFAULT

#if RF_ANTENNA_DELAY == RF_ANTENNA_DELAY_DEFAULT
  #warning "RF_ANTENNA_DELAY(DW1000) is the uncalibrated default. Run the calibration procedure."
#endif

inline void applyRfConfigDW1000() {
    DW1000.setChannel(RF_CHANNEL);
    DW1000.setPreambleCode(RF_PREAMBLE_CODE);
    DW1000.setDataRate(RF_DATA_RATE);
    DW1000.setPreambleLength(RF_PREAMBLE_LENGTH);
    DW1000.setAntennaDelay(RF_ANTENNA_DELAY);
    DW1000.commitConfiguration();
}

#endif // RF_CONFIG_DW1000_H
