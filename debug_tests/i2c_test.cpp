#include <Arduino.h>
#include <Wire.h>
#include "i2c_test.h"

namespace dbg_i2c {
    void run() {
        Serial.println("Starting I2C scan...");
        Wire.begin();
        uint8_t count = 0;
        for (uint8_t addr = 1; addr < 127; ++addr) {
            Wire.beginTransmission(addr);
            uint8_t err = Wire.endTransmission();
            if (err == 0) {
                Serial.print("Found I2C device at 0x");
                if (addr < 16) Serial.print('0');
                Serial.println(addr, HEX);
                ++count;
            }
        }
        if (count == 0) Serial.println("No I2C devices found.");
        Serial.println("I2C scan complete.");
        delay(500);
    }
}
