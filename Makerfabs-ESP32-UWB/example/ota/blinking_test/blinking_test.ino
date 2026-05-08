#include <Arduino.h>


void setup()
{
    Serial.begin(115200);

    Serial.println("OTA Test Firmware");
}

void loop()
{
    Serial.println("OTA Testing...");
    delay(500);
}
