#include <Arduino.h>
#include "audio_output_test.h"

// ESP32 DAC pins: 25 (DAC1) and 26 (DAC2). Adjust as needed.
static const int DAC_PIN = 25;

namespace dbg_audio {
    void run() {
        Serial.println("Starting audio output test (square wave on DAC)...");
        pinMode(DAC_PIN, OUTPUT);
        // Generate a short square-ish waveform using dacWrite
        for (int t = 0; t < 500; ++t) {
            dacWrite(DAC_PIN, 200);
            delayMicroseconds(500);
            dacWrite(DAC_PIN, 55);
            delayMicroseconds(500);
        }
        dacWrite(DAC_PIN, 0);
        Serial.println("Audio output test complete.");
        delay(100);
    }
}
