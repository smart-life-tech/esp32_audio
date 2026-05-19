// MAS6116 GPIO toggle test (Arduino .ino)

static const int MAS6116_PIN = 14; // adjust for your wiring

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("MAS6116 control line toggle test (Arduino .ino)");
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
}

void loop() {
  delay(1000);
}
