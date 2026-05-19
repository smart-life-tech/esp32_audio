#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("I2C Scanner (Arduino .ino)");
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
}

void loop() {
  delay(1000);
}
