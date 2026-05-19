// PGA2311 SPI Test
// A1 = XCS (chip select)
// D11 = COPI/MOSI (DATA)
// D13 = SCK (CCLK)
#include <SPI.h>

static const int PGA2311_XCS_PIN = A1;
static const int PGA2311_DATA_PIN = 11; // MOSI
static const int PGA2311_CLK_PIN = 13;  // SCK

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("=== PGA2311 SPI TEST ===");
  
  pinMode(PGA2311_XCS_PIN, OUTPUT);
  digitalWrite(PGA2311_XCS_PIN, HIGH); // CS inactive
  
  SPI.begin(PGA2311_CLK_PIN, -1, PGA2311_DATA_PIN); // SCLK, MISO (unused), MOSI
  SPI.setFrequency(1000000); // 1 MHz
  
  Serial.println("PGA2311 SPI initialized.");
  Serial.println("Testing chip select toggle...");
  
  for (int i = 0; i < 5; ++i) {
    digitalWrite(PGA2311_XCS_PIN, LOW);
    Serial.print("CS LOW");
    delay(200);
    digitalWrite(PGA2311_XCS_PIN, HIGH);
    Serial.println(", CS HIGH");
    delay(200);
  }
  
  Serial.println("PGA2311 SPI test complete.");
}

void loop() {
  delay(1000);
}
