#include <Arduino.h>

#define CLK_PIN 12
#define DT_PIN A3
#define SW_PIN A0
#define SRC_BTN_PIN A2

#define DIRECTION_CW 0
#define DIRECTION_CCW 1

volatile int counter = 0;
volatile int direction = DIRECTION_CW;
volatile uint8_t lastEncoderState = 0;

int prev_counter = 0;
int last_button_state = HIGH;
int last_source_state = HIGH;
unsigned long last_button_time = 0;
unsigned long last_source_time = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 50;

void encoderISR() {
  uint8_t a = digitalRead(CLK_PIN);
  uint8_t b = digitalRead(DT_PIN);
  uint8_t state = (a << 1) | b;

  if (state == lastEncoderState) {
    return;
  }

  uint8_t diff = (lastEncoderState - state) & 0x03;
  if (diff == 1) {
    counter++;
    direction = DIRECTION_CW;
  } else if (diff == 3) {
    counter--;
    direction = DIRECTION_CCW;
  }

  lastEncoderState = state;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(SRC_BTN_PIN, INPUT_PULLUP);

  lastEncoderState = (digitalRead(CLK_PIN) << 1) | digitalRead(DT_PIN);

  attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT_PIN), encoderISR, CHANGE);

  Serial.println("Encoder test on D12/A3, push=A0, source=A2");
}

void loop() {
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

  int currentCount;
  int currentDirection;
  noInterrupts();
  currentCount = counter;
  currentDirection = direction;
  interrupts();

  if (prev_counter != currentCount) {
    Serial.print("DIRECTION: ");
    Serial.print(currentDirection == DIRECTION_CW ? "Clockwise" : "Counter-clockwise");
    Serial.print(" | COUNTER: ");
    Serial.println(currentCount);
    prev_counter = currentCount;
  }

  delay(10);
}
