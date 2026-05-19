// Relay Test: Toggle all 8 relays
// D2-D9 are relay pins (8 relays for Phono, CD, Aux1, Aux2, DVD, Tuner, Tape1, Tape2)

static const int RELAY_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 9};
static const char* RELAY_NAMES[8] = {"Phono", "CD", "Aux1", "Aux2", "DVD", "Tuner", "Tape1", "Tape2"};

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("=== RELAY TEST ===");
  
  // Initialize all relay pins as outputs
  for (int i = 0; i < 8; ++i) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], HIGH); // Default OFF (active-low)
  }
  
  // Test each relay one at a time
  for (int i = 0; i < 8; ++i) {
    Serial.print("Activating relay ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(RELAY_NAMES[i]);
    
    // Turn ON selected relay
    digitalWrite(RELAY_PINS[i], LOW);
    delay(500);
    
    // Turn OFF selected relay
    digitalWrite(RELAY_PINS[i], HIGH);
    delay(300);
  }
  
  Serial.println("Relay test complete.");
}

void loop() {
  delay(1000);
}
