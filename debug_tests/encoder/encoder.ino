// Encoder and Buttons Test
// D12 = Encoder A
// A3 = Encoder B
// A0 = Encoder push switch
// A2 = Source button
#include <Arduino.h>
static const int ENCODER_A_PIN = 12;
static const int ENCODER_B_PIN = A3;
static const int ENCODER_PUSH_PIN = A0;
static const int SOURCE_BTN_PIN = A2;

volatile int encoderCount = 0;
volatile uint32_t lastIsrMs = 0;

void IRAM_ATTR encoderISR() {
  // Simple debounce: ignore rapid successive calls
  uint32_t now = millis();
  if ((now - lastIsrMs) < 3) {
    return;
  }
  lastIsrMs = now;

  // Read both pins to determine direction
  bool a = digitalRead(ENCODER_A_PIN);
  bool b = digitalRead(ENCODER_B_PIN);

  // Quadrature logic: if A and B same -> CW, else CCW
  if (a == b) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("=== ENCODER & BUTTONS TEST ===");
  
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PUSH_PIN, INPUT_PULLUP);
  pinMode(SOURCE_BTN_PIN, INPUT_PULLUP);
  
  // Attach interrupt to both encoder pins for responsive detection
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);
  
  Serial.println("Encoder on D12 (A) / A3 (B)");
  Serial.println("Encoder push on A0");
  Serial.println("Source button on A2");
  Serial.println("Rotate encoder or press buttons...");
}

int lastCount = 0;
int lastEncoderPush = HIGH;
int lastSourceBtn = HIGH;

void loop() {
  if (encoderCount != lastCount) {
    int delta = encoderCount - lastCount;
    Serial.print("Encoder moved: ");
    Serial.print(delta > 0 ? "CW +" : "CCW ");
    Serial.print(delta);
    Serial.print("  Total count: ");
    Serial.println(encoderCount);
    lastCount = encoderCount;
  }
  
  int enPush = digitalRead(ENCODER_PUSH_PIN);
  if (enPush != lastEncoderPush) {
    lastEncoderPush = enPush;
    if (enPush == LOW) {
      Serial.println("Encoder push button pressed!");
    }
    delay(20); // debounce
  }
  
  int srcBtn = digitalRead(SOURCE_BTN_PIN);
  if (srcBtn != lastSourceBtn) {
    lastSourceBtn = srcBtn;
    if (srcBtn == LOW) {
      Serial.println("Source button pressed!");
    }
    delay(20); // debounce
  }
  
  delay(50);
}
