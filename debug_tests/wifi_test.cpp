#include <Arduino.h>
#include <WiFi.h>
#include "wifi_test.h"

namespace dbg_wifi {
    void run() {
        Serial.println("Starting WiFi AP for quick test...");
        const char *apName = "ESP32-debug";
        bool ok = WiFi.softAP(apName);
        if (!ok) {
            Serial.println("Failed to start AP");
        } else {
            IPAddress ip = WiFi.softAPIP();
            Serial.print("AP started: ");
            Serial.println(apName);
            Serial.print("AP IP: ");
            Serial.println(ip);
            delay(200);
        }
        Serial.println("WiFi AP test complete.");
        delay(100);
    }
}
