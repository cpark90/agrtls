/*
 * Variant: tag_dw3000_fast_filtered
 * Tag / DW3000 / FAST mode / software filtering.
 *
 * FAST mode (6.8Mbps) for fast updates + RangeFilter to smooth jitter.
 * Fill in the DW3000 low-level API parts with the real library.
 */
#include <Arduino.h>
#include <SPI.h>
#include "rf_config_dw3000.h"
#include "logging.h"
#include "range_filter.h"

// TODO: include the real DW3000 library header
// #include "dw3000.h"

RangeFilter rangeFilter(0.3f, 0.5f);  // alpha, outlier threshold (m)

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# tag_dw3000_fast_filtered");

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);

    // TODO: DW3000 init + dwt_configure(&dw3000_config) (FAST parameters)
    //       dwt_setantennadelay(DW3000_ANTENNA_DELAY);
}

void loop() {
    // TODO: measure raw range via the DS-TWR initiator state machine
    // float raw = ...;  float rxp = ...;
    // float filtered = rangeFilter.update(raw);
    // char devId[8]; shortAddrToId(anchorShortAddr, devId, sizeof(devId));
    // logRange(devId, filtered, rxp);
}
