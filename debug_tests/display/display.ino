// TFT Display Test (HSD028309 / ILI9341)
// A4 = TFT CS
// A5 = TFT DC
// D11 = COPI/MOSI (SID)
// D13 = SCK (CLK)
// TFT RST = board RESET

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

static const int TFT_CS_PIN = A4;
static const int TFT_DC_PIN = A5;
static const int TFT_MOSI_PIN = 11;
static const int TFT_SCK_PIN = 13;

Adafruit_ILI9341 tft(TFT_CS_PIN, TFT_DC_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("=== TFT DISPLAY TEST ===");
  
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  
  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("TFT Display Test");
  
  tft.setCursor(10, 40);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_GREEN);
  tft.println("Testing color output...");
  delay(500);
  
  // Draw color bars
  tft.fillRect(0, 60, 80, 40, ILI9341_RED);
  tft.fillRect(80, 60, 80, 40, ILI9341_GREEN);
  tft.fillRect(160, 60, 80, 40, ILI9341_BLUE);
  tft.fillRect(240, 60, 80, 40, ILI9341_YELLOW);
  
  delay(1000);
  tft.fillScreen(ILI9341_BLACK);
  
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_CYAN);
  tft.println("Display OK!");
  
  Serial.println("TFT display test complete.");
}

void loop() {
  delay(1000);
}
