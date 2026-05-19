#include <Arduino.h>
#include <Wire.h>
#include "pga2311_test.h"

// Default PGA2311 I2C address used as an example; update if different.
static const uint8_t PGA2311_ADDR = 0x44;

namespace dbg_pga2311 {
    void run() {
        Serial.println("Probing PGA2311 at I2C address 0x44...");
        Wire.begin();
        Wire.beginTransmission(PGA2311_ADDR);
        Wire.write(0x00); // example register/command; harmless probe
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.println("PGA2311 ACKed the probe (device present at 0x44).");
        } else {
            Serial.print("No ack from 0x44, error=");
            Serial.println(err);
        }
        Serial.println("PGA2311 probe complete.");
        delay(200);
    }
}
