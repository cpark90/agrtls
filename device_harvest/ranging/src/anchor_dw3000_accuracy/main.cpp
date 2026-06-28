/*
 * Variant: anchor_dw3000_accuracy
 * Anchor / DW3000 / ACCURACY mode.
 *
 * Note: DW3000's library API is completely different from DW1000.
 *   The Makerfabs DW3000 library (NConcepts) typically uses the low-level dwt_* API or its own
 *   wrappers, like the range_tx/range_rx examples.
 *   This is a structural skeleton only -- fill it in with the real library functions.
 *   (it may not have a high-level callback API like DW1000's DW1000Ranging)
 */
#include <Arduino.h>
#include <SPI.h>
#include "rf_config_dw3000.h"
#include "logging.h"

// TODO: include the real DW3000 library header
// #include "dw3000.h"

void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("# anchor_dw3000_accuracy");

    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);

    // TODO: DW3000 init (example flow)
    //   spiBegin(PIN_IRQ, PIN_RST);
    //   spiSelect(PIN_SS);
    //   if (dwt_initialise(...) == DWT_ERROR) { ... }
    //   dwt_configure(&dw3000_config);  // use the DW3000_* values from rf_config_dw3000.h
    //   dwt_setantennadelay(DW3000_ANTENNA_DELAY);
    //
    // Then the DS-TWR responder routine (recv POLL -> send RESP -> recv FINAL -> compute range).
    // Emit the computed range / RX power via logRange:
    //   char devId[8]; shortAddrToId(tagShortAddr, devId, sizeof(devId));
    //   logRange(devId, range, rxp);
    // For the anchor number, see docs/VARIANTS.md "Short Address Assignment Rule".
}

void loop() {
    // TODO: DS-TWR responder state machine
    // when a range is produced:
    //   char devId[8]; shortAddrToId(tagShortAddr, devId, sizeof(devId));
    //   logRange(devId, range_m, rxPower_dBm);
}
