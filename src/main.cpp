#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>

#define DECODE_RC5
#include <IRremote.hpp>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

namespace {

constexpr uint8_t kSourceCount = 8;
constexpr uint8_t kRelayPins[kSourceCount] = {D2, D3, D4, D5, D6, D7, D8, D9};
constexpr const char *kSourceNames[kSourceCount] = {
    "Phono",
    "CD",
    "Aux 1",
    "Aux 2",
    "DVD",
    "Tuner",
    "Tape 1",
    "Tape 2",
};

constexpr uint8_t kIrPin = D10;
constexpr uint8_t kSpiMosiPin = D11;
constexpr uint8_t kSpiSckPin = D13;
constexpr uint8_t kMasCsPin = A1;
constexpr uint8_t kEncoderPinA = D12;
constexpr uint8_t kEncoderPinB = A3;
constexpr uint8_t kEncoderButtonPin = A0;
constexpr uint8_t kSourceButtonPin = A2;
constexpr uint8_t kTftCsPin = A4;
constexpr uint8_t kTftDcPin = A5;
constexpr int8_t kTftRstPin = -1;

constexpr uint8_t kRelayOnLevel = LOW;
constexpr uint8_t kRelayOffLevel = HIGH;

constexpr uint8_t kMasRegMask = 0x0F << 3;
constexpr uint8_t kMasReadBit = 0x01 << 2;
constexpr uint8_t kMasLeftReg = 0x0D << 3;
constexpr uint8_t kMasRightReg = 0x0E << 3;
constexpr uint8_t kMasWritePrefix = static_cast<uint8_t>(~(kMasRegMask | kMasReadBit));
constexpr uint8_t kMuteCode = 171;
constexpr uint8_t kMinVolumeCode = 0;
constexpr uint8_t kMaxVolumeCode = 234;
constexpr uint8_t kDefaultVolumeCode = 120;

constexpr uint16_t kButtonDebounceMs = 35;
constexpr uint16_t kEncoderDebounceMs = 2;
constexpr uint8_t kDisplayRotation = 1;

Adafruit_ILI9341 tft(kTftCsPin, kTftDcPin, kTftRstPin);
Preferences preferences;

uint8_t sourceVolumes[kSourceCount];
uint8_t currentSource = 0;
uint8_t currentVolume = kDefaultVolumeCode;
uint8_t muteRestoreVolume = kDefaultVolumeCode;
bool muted = false;
bool uiDirty = true;

uint8_t encoderLastA = HIGH;
uint32_t encoderLastMoveMs = 0;

struct DebouncedButton {
  uint8_t pin;
  bool stableState;
  bool lastReading;
  uint32_t lastChangeMs;
};

DebouncedButton encoderButton{kEncoderButtonPin, HIGH, HIGH, 0};
DebouncedButton sourceButton{kSourceButtonPin, HIGH, HIGH, 0};

uint8_t masWriteAddress(uint8_t reg) {
  return static_cast<uint8_t>((reg & kMasRegMask) | kMasWritePrefix);
}

int16_t volumeCodeToTenthsDb(uint8_t code) {
  return static_cast<int16_t>(155) - static_cast<int16_t>(code) * 5;
}

uint8_t clampVolumeCode(int value) {
  if (value < kMinVolumeCode) {
    return kMinVolumeCode;
  }
  if (value > kMaxVolumeCode) {
    return kMaxVolumeCode;
  }
  return static_cast<uint8_t>(value);
}

void saveState() {
  preferences.putBytes("vols", sourceVolumes, sizeof(sourceVolumes));
  preferences.putUChar("source", currentSource);
  preferences.putBool("muted", muted);
  preferences.putUChar("muteRestore", muteRestoreVolume);
}

void loadState() {
  for (uint8_t index = 0; index < kSourceCount; ++index) {
    sourceVolumes[index] = kDefaultVolumeCode;
  }

  if (preferences.getBytesLength("vols") == sizeof(sourceVolumes)) {
    preferences.getBytes("vols", sourceVolumes, sizeof(sourceVolumes));
  }

  currentSource = preferences.getUChar("source", 0);
  if (currentSource >= kSourceCount) {
    currentSource = 0;
  }

  muted = preferences.getBool("muted", false);
  muteRestoreVolume = preferences.getUChar("muteRestore", sourceVolumes[currentSource]);
  currentVolume = sourceVolumes[currentSource];
}

void writeMasVolume(uint8_t volumeCode) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(kMasCsPin, LOW);
  SPI.transfer(masWriteAddress(kMasLeftReg));
  SPI.transfer(volumeCode);
  SPI.transfer(masWriteAddress(kMasRightReg));
  SPI.transfer(volumeCode);
  digitalWrite(kMasCsPin, HIGH);
  SPI.endTransaction();
}

void applyCurrentVolume() {
  currentVolume = clampVolumeCode(currentVolume);
  sourceVolumes[currentSource] = currentVolume;
  writeMasVolume(currentVolume);
  saveState();
  uiDirty = true;
}

void setMuted(bool shouldMute) {
  if (shouldMute) {
    if (!muted) {
      muteRestoreVolume = currentVolume;
    }
    muted = true;
    writeMasVolume(kMuteCode);
  } else {
    muted = false;
    currentVolume = clampVolumeCode(muteRestoreVolume);
    sourceVolumes[currentSource] = currentVolume;
    writeMasVolume(currentVolume);
  }
  saveState();
  uiDirty = true;
}

void toggleMute() {
  setMuted(!muted);
}

void setRelaySource(uint8_t sourceIndex) {
  for (uint8_t index = 0; index < kSourceCount; ++index) {
    digitalWrite(kRelayPins[index], kRelayOffLevel);
  }

  digitalWrite(kRelayPins[sourceIndex], kRelayOnLevel);
}

void setSource(uint8_t sourceIndex) {
  if (sourceIndex >= kSourceCount) {
    return;
  }

  currentSource = sourceIndex;
  setRelaySource(currentSource);

  currentVolume = sourceVolumes[currentSource];
  muteRestoreVolume = currentVolume;

  if (muted) {
    writeMasVolume(kMuteCode);
  } else {
    writeMasVolume(currentVolume);
  }

  saveState();
  uiDirty = true;
}

void nextSource() {
  setSource(static_cast<uint8_t>((currentSource + 1) % kSourceCount));
}

void previousSource() {
  setSource(static_cast<uint8_t>((currentSource + kSourceCount - 1) % kSourceCount));
}

void changeVolume(int delta) {
  if (muted) {
    setMuted(false);
  }

  currentVolume = clampVolumeCode(static_cast<int>(currentVolume) + delta);
  sourceVolumes[currentSource] = currentVolume;
  writeMasVolume(currentVolume);
  saveState();
  uiDirty = true;
}

bool updateButton(DebouncedButton &button) {
  bool reading = digitalRead(button.pin);

  if (reading != button.lastReading) {
    button.lastChangeMs = millis();
    button.lastReading = reading;
  }

  if ((millis() - button.lastChangeMs) >= kButtonDebounceMs && reading != button.stableState) {
    button.stableState = reading;
    if (button.stableState == LOW) {
      return true;
    }
  }

  return false;
}

void handleRc5Command(uint8_t command) {
  switch (command) {
    case 1:
      setSource(0);
      break;
    case 2:
      setSource(1);
      break;
    case 3:
      setSource(2);
      break;
    case 4:
      setSource(3);
      break;
    case 5:
      setSource(4);
      break;
    case 6:
      setSource(5);
      break;
    case 7:
      setSource(6);
      break;
    case 8:
      setSource(7);
      break;
    case 13:
      toggleMute();
      break;
    case 16:
      changeVolume(+1);
      break;
    case 17:
      changeVolume(-1);
      break;
    case 26:
      previousSource();
      break;
    case 27:
      nextSource();
      break;
    default:
      break;
  }
}

void pollIrReceiver() {
  if (!IrReceiver.decode()) {
    return;
  }

  const auto &data = IrReceiver.decodedIRData;
  if (data.protocol == RC5) {
    if (data.address == 0x10 || data.address == 0x14) {
      handleRc5Command(data.command & 0x3F);
    }
  }

  IrReceiver.resume();
}

void pollEncoder() {
  bool encoderA = digitalRead(kEncoderPinA);
  if (encoderA != encoderLastA) {
    if ((millis() - encoderLastMoveMs) >= kEncoderDebounceMs && encoderA == LOW) {
      if (digitalRead(kEncoderPinB) == HIGH) {
        changeVolume(+1);
      } else {
        changeVolume(-1);
      }
      encoderLastMoveMs = millis();
    }
    encoderLastA = encoderA;
  }
}

void drawUi() {
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(12, 12);
  tft.print("Source ");
  tft.print(currentSource + 1);
  tft.print("/8");

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(4);
  tft.setCursor(12, 56);
  tft.print(kSourceNames[currentSource]);

  tft.setTextSize(3);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setCursor(12, 120);
  tft.print("Vol: ");
  tft.print(currentVolume);

  char dbBuffer[16];
  int16_t dbTenths = muted ? static_cast<int16_t>(-700) : volumeCodeToTenthsDb(currentVolume);
  int16_t whole = dbTenths / 10;
  int16_t fraction = abs(dbTenths % 10);
  snprintf(dbBuffer, sizeof(dbBuffer), "%s%ld.%ld dB", (dbTenths >= 0) ? "+" : "", static_cast<long>(whole), static_cast<long>(fraction));

  tft.setCursor(12, 160);
  tft.print(dbBuffer);

  tft.setTextColor(muted ? ILI9341_RED : ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(4);
  tft.setCursor(12, 202);
  tft.print(muted ? "MUTED" : "LIVE");
}

void pollButtons() {
  if (updateButton(encoderButton)) {
    toggleMute();
  }

  if (updateButton(sourceButton)) {
    nextSource();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);

  pinMode(kMasCsPin, OUTPUT);
  digitalWrite(kMasCsPin, HIGH);

  SPI.begin(kSpiSckPin, -1, kSpiMosiPin);

  for (uint8_t index = 0; index < kSourceCount; ++index) {
    pinMode(kRelayPins[index], OUTPUT);
    digitalWrite(kRelayPins[index], kRelayOffLevel);
  }

  pinMode(kEncoderPinA, INPUT_PULLUP);
  pinMode(kEncoderPinB, INPUT_PULLUP);
  pinMode(kEncoderButtonPin, INPUT_PULLUP);
  pinMode(kSourceButtonPin, INPUT_PULLUP);
  pinMode(kIrPin, INPUT_PULLUP);

  preferences.begin("audioctl", false);
  loadState();

  tft.begin();
  tft.setRotation(kDisplayRotation);
  tft.fillScreen(ILI9341_BLACK);

  IrReceiver.begin(kIrPin, DISABLE_LED_FEEDBACK);

  setRelaySource(currentSource);
  if (muted) {
    writeMasVolume(kMuteCode);
  } else {
    writeMasVolume(currentVolume);
  }

  uiDirty = true;
  drawUi();
  uiDirty = false;
}

void loop() {
  pollIrReceiver();
  pollEncoder();
  pollButtons();

  if (uiDirty) {
    drawUi();
    uiDirty = false;
  }
}