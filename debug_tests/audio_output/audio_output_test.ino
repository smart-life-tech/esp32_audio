// ESP32 DAC output test (Arduino .ino)

static const int DAC_PIN = 25; // ESP32 DAC1

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Audio output test (Arduino .ino) -- DAC square-ish wave");
  pinMode(DAC_PIN, OUTPUT);
  for (int t = 0; t < 500; ++t) {
    dacWrite(DAC_PIN, 200);
    delayMicroseconds(500);
    dacWrite(DAC_PIN, 55);
    delayMicroseconds(500);
  }
  dacWrite(DAC_PIN, 0);
  Serial.println("Audio output test complete.");
}

void loop() {
  delay(1000);
}
