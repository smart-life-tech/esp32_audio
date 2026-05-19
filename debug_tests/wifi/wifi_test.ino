#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("WiFi AP test (Arduino .ino)");
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
  }
  Serial.println("WiFi AP test complete.");
}

void loop() {
  delay(1000);
}
