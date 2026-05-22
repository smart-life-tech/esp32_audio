#include <Arduino.h>
#include <Encoder.h>  // https://github.com/PaulStoffregen/Encoder/tree/master

#define CLK_PIN 12
#define DT_PIN A3
#define SW_PIN A0
#define SRC_BTN_PIN A2

Encoder myEnc(CLK_PIN, DT_PIN);
long oldPosition = -999;

int last_button_state = HIGH;
int last_source_state = HIGH;
unsigned long last_button_time = 0;
unsigned long last_source_time = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 50;

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(SRC_BTN_PIN, INPUT_PULLUP);

  Serial.println("Basic Encoder Test:");
  Serial.println("Encoder on D12/A3, push=A0, source=A2");
}

void loop() {
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    Serial.print("Encoder position: ");
    Serial.println(newPosition);
  }

  unsigned long now = millis();
  int button_state = digitalRead(SW_PIN);
  int source_state = digitalRead(SRC_BTN_PIN);

  if (button_state != last_button_state) {
    last_button_time = now;
    last_button_state = button_state;
  }

  if (button_state == LOW && (now - last_button_time) >= BUTTON_DEBOUNCE_MS) {
    Serial.println("Encoder push button pressed");
    while (digitalRead(SW_PIN) == LOW) {
      delay(5);
    }
    last_button_state = HIGH;
    last_button_time = millis();
  }

  if (source_state != last_source_state) {
    last_source_time = now;
    last_source_state = source_state;
  }

  if (source_state == LOW && (now - last_source_time) >= BUTTON_DEBOUNCE_MS) {
    Serial.println("Source button pressed");
    while (digitalRead(SRC_BTN_PIN) == LOW) {
      delay(5);
    }
    last_source_state = HIGH;
    last_source_time = millis();
  }

  delay(10);
}
