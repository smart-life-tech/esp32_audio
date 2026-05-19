#include <Arduino.h>
#include "mas6116_test.h"

// Example control pin for MAS6116 (adjust to your wiring)
static const int MAS6116_PIN = 14;

namespace dbg_mas6116 {
    void run() {
        Serial.println("MAS6116 control line toggle test...");
        pinMode(MAS6116_PIN, OUTPUT);
        for (int i = 0; i < 6; ++i) {
            digitalWrite(MAS6116_PIN, i % 2);
            Serial.print("Set pin ");
            Serial.print(MAS6116_PIN);
            Serial.print(" = ");
            Serial.println(i % 2);
            delay(200);
        }
        digitalWrite(MAS6116_PIN, LOW);
        Serial.println("MAS6116 toggle test complete.");
        delay(100);
    }
}
