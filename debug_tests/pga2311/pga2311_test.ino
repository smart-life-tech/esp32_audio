#include <Wire.h>

// Default PGA2311 I2C address used as an example; update if different.
static const uint8_t PGA2311_ADDR = 0x44;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("PGA2311 I2C probe (Arduino .ino)");
  Wire.begin();
  Wire.beginTransmission(PGA2311_ADDR);
  Wire.write(0x00); // example harmless probe
  uint8_t err = Wire.endTransmission();
  if (err == 0) {
    Serial.println("PGA2311 ACKed the probe (device present at 0x44).");
  } else {
    Serial.print("No ack from 0x44, error=");
    Serial.println(err);
  }
  Serial.println("PGA2311 probe complete.");
}

void loop() {
  delay(1000);
}
