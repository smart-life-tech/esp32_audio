#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>

#define DECODE_RC5
#include <IRremote.hpp>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Debug configuration
const bool kDebugEnabled = true; // Set to false to disable all debug output

namespace
{

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

  struct DebouncedButton
  {
    uint8_t pin;
    bool stableState;
    bool lastReading;
    uint32_t lastChangeMs;
  };

  DebouncedButton encoderButton{kEncoderButtonPin, HIGH, HIGH, 0};
  DebouncedButton sourceButton{kSourceButtonPin, HIGH, HIGH, 0};

  // Debug helper function
  void debugLog(const char *format, ...)
  {
    if (!kDebugEnabled)
      return;
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    uint32_t ms = millis();
    Serial.printf("[%10lu] %s\n", ms, buffer);
  }

  uint8_t masWriteAddress(uint8_t reg)
  {
    return static_cast<uint8_t>((reg & kMasRegMask) | kMasWritePrefix);
  }

  uint8_t masReadAddress(uint8_t reg)
  {
    return static_cast<uint8_t>((reg & kMasRegMask) | kMasReadBit | kMasWritePrefix);
  }

  uint8_t readMasVolume(uint8_t reg)
  {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(kMasCsPin, LOW);
    uint8_t readAddr = masReadAddress(reg);
    SPI.transfer(readAddr);
    uint8_t volumeCode = SPI.transfer(0x00);
    digitalWrite(kMasCsPin, HIGH);
    SPI.endTransaction();
    return volumeCode;
  }

  int16_t volumeCodeToTenthsDb(uint8_t code)
  {
    return static_cast<int16_t>(155) - static_cast<int16_t>(code) * 5;
  }

  uint8_t clampVolumeCode(int value)
  {
    if (value < kMinVolumeCode)
    {
      return kMinVolumeCode;
    }
    if (value > kMaxVolumeCode)
    {
      return kMaxVolumeCode;
    }
    return static_cast<uint8_t>(value);
  }

  void saveState()
  {
    preferences.putBytes("vols", sourceVolumes, sizeof(sourceVolumes));
    preferences.putUChar("source", currentSource);
    preferences.putBool("muted", muted);
    preferences.putUChar("muteRestore", muteRestoreVolume);
    debugLog("PERSIST: state saved (src=%u muted=%s vol=%u)", currentSource, muted ? "yes" : "no", currentVolume);
  }

  void loadState()
  {
    for (uint8_t index = 0; index < kSourceCount; ++index)
    {
      sourceVolumes[index] = kDefaultVolumeCode;
    }

    if (preferences.getBytesLength("vols") == sizeof(sourceVolumes))
    {
      preferences.getBytes("vols", sourceVolumes, sizeof(sourceVolumes));
      debugLog("PERSIST: loaded 8 source volumes from NVS");
    }
    else
    {
      debugLog("PERSIST: no saved volumes found, using defaults");
    }

    currentSource = preferences.getUChar("source", 0);
    if (currentSource >= kSourceCount)
    {
      debugLog("PERSIST: invalid source %u, defaulting to 0", currentSource);
      currentSource = 0;
    }

    muted = preferences.getBool("muted", false);
    muteRestoreVolume = preferences.getUChar("muteRestore", sourceVolumes[currentSource]);
    currentVolume = sourceVolumes[currentSource];

    int16_t dbTenths = volumeCodeToTenthsDb(currentVolume);
    debugLog("PERSIST: restored state src=%u (%s) muted=%s code=%u (dB=%d.%d)",
             currentSource, kSourceNames[currentSource], muted ? "yes" : "no",
             currentVolume, dbTenths / 10, abs(dbTenths % 10));
  }

  void writeMasVolume(uint8_t volumeCode)
  {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(kMasCsPin, LOW);
    SPI.transfer(masWriteAddress(kMasLeftReg));
    SPI.transfer(volumeCode);
    SPI.transfer(masWriteAddress(kMasRightReg));
    SPI.transfer(volumeCode);
    digitalWrite(kMasCsPin, HIGH);
    SPI.endTransaction();

    int16_t dbTenths = volumeCodeToTenthsDb(volumeCode);
    debugLog("MAS6116_WRITE: code=%3u -> dB=%d.%d", volumeCode, dbTenths / 10, abs(dbTenths % 10));
  }

  void applyCurrentVolume()
  {
    currentVolume = clampVolumeCode(currentVolume);
    sourceVolumes[currentSource] = currentVolume;
    writeMasVolume(currentVolume);
    saveState();
    uiDirty = true;
  }

  void setMuted(bool shouldMute)
  {
    if (shouldMute)
    {
      if (!muted)
      {
        muteRestoreVolume = currentVolume;
        debugLog("MUTE: saving restore volume code=%u", muteRestoreVolume);
      }
      muted = true;
      debugLog("MUTE: output muted (-70 dB)");
      writeMasVolume(kMuteCode);
    }
    else
    {
      muted = false;
      currentVolume = clampVolumeCode(muteRestoreVolume);
      sourceVolumes[currentSource] = currentVolume;
      int16_t dbTenths = volumeCodeToTenthsDb(currentVolume);
      debugLog("UNMUTE: restoring to code=%u (dB=%d.%d)", currentVolume, dbTenths / 10, abs(dbTenths % 10));
      writeMasVolume(currentVolume);
    }
    saveState();
    uiDirty = true;
  }

  void toggleMute()
  {
    setMuted(!muted);
  }

  void setRelaySource(uint8_t sourceIndex)
  {
    for (uint8_t index = 0; index < kSourceCount; ++index)
    {
      digitalWrite(kRelayPins[index], kRelayOffLevel);
    }

    digitalWrite(kRelayPins[sourceIndex], kRelayOnLevel);
    debugLog("Relay: activated source %u (%s) on pin D%d", sourceIndex, kSourceNames[sourceIndex], kRelayPins[sourceIndex]);
  }

  void setSource(uint8_t sourceIndex)
  {
    if (sourceIndex >= kSourceCount)
    {
      debugLog("ERROR: source index %u out of range (0-%u)", sourceIndex, kSourceCount - 1);
      return;
    }

    currentSource = sourceIndex;
    debugLog("SOURCE: switching to %s (index %u)", kSourceNames[currentSource], sourceIndex);
    setRelaySource(currentSource);

    currentVolume = sourceVolumes[currentSource];
    muteRestoreVolume = currentVolume;

    if (muted)
    {
      debugLog("SOURCE: source is muted, keeping -70 dB");
      writeMasVolume(kMuteCode);
    }
    else
    {
      int16_t dbTenths = volumeCodeToTenthsDb(currentVolume);
      debugLog("SOURCE: applying stored volume code=%u (dB=%d.%d)", currentVolume, dbTenths / 10, abs(dbTenths % 10));
      writeMasVolume(currentVolume);
    }

    saveState();
    uiDirty = true;
  }

  void nextSource()
  {
    setSource(static_cast<uint8_t>((currentSource + 1) % kSourceCount));
  }

  void previousSource()
  {
    setSource(static_cast<uint8_t>((currentSource + kSourceCount - 1) % kSourceCount));
  }

  void changeVolume(int delta)
  {
    if (muted)
    {
      debugLog("VOLUME: unmuting first (was at -70 dB)");
      setMuted(false);
      return; // Next encoder turn will adjust volume
    }

    uint8_t oldVolume = currentVolume;
    currentVolume = clampVolumeCode(static_cast<int>(currentVolume) + delta);
    sourceVolumes[currentSource] = currentVolume;

    int16_t dbOld = volumeCodeToTenthsDb(oldVolume);
    int16_t dbNew = volumeCodeToTenthsDb(currentVolume);
    debugLog("VOLUME: %+d step  code %u->%u  dB %d.%d->%d.%d", delta, oldVolume, currentVolume,
             dbOld / 10, abs(dbOld % 10), dbNew / 10, abs(dbNew % 10));

    writeMasVolume(currentVolume);
    saveState();
    uiDirty = true;
  }

  bool updateButton(DebouncedButton &button)
  {
    bool reading = digitalRead(button.pin);

    if (reading != button.lastReading)
    {
      button.lastChangeMs = millis();
      button.lastReading = reading;
    }

    if ((millis() - button.lastChangeMs) >= kButtonDebounceMs && reading != button.stableState)
    {
      button.stableState = reading;
      if (button.stableState == LOW)
      {
        return true;
      }
    }

    return false;
  }

  void handleRc5Command(uint8_t command)
  {
    debugLog("IR: RC5 command 0x%02X received", command);
    switch (command)
    {
    case 1:
      debugLog("IR: -> select Phono");
      setSource(0);
      break;
    case 2:
      debugLog("IR: -> select CD");
      setSource(1);
      break;
    case 3:
      debugLog("IR: -> select Aux 1");
      setSource(2);
      break;
    case 4:
      debugLog("IR: -> select Aux 2");
      setSource(3);
      break;
    case 5:
      debugLog("IR: -> select DVD");
      setSource(4);
      break;
    case 6:
      debugLog("IR: -> select Tuner");
      setSource(5);
      break;
    case 7:
      debugLog("IR: -> select Tape 1");
      setSource(6);
      break;
    case 8:
      debugLog("IR: -> select Tape 2");
      setSource(7);
      break;
    case 13:
      debugLog("IR: -> toggle mute");
      toggleMute();
      break;
    case 16:
      debugLog("IR: -> volume up");
      changeVolume(+1);
      break;
    case 17:
      debugLog("IR: -> volume down");
      changeVolume(-1);
      break;
    case 26:
      debugLog("IR: -> previous source");
      previousSource();
      break;
    case 27:
      debugLog("IR: -> next source");
      nextSource();
      break;
    default:
      debugLog("IR: -> unknown command (ignored)");
      break;
    }
  }

  void pollIrReceiver()
  {
    if (!IrReceiver.decode())
    {
      return;
    }

    const auto &data = IrReceiver.decodedIRData;
    if (data.protocol == RC5)
    {
      if (data.address == 0x10 || data.address == 0x14)
      {
        handleRc5Command(data.command & 0x3F);
      }
      else
      {
        debugLog("IR: RC5 address 0x%02X ignored (expected 0x10 or 0x14)", data.address);
      }
    }
    else
    {
      debugLog("IR: protocol %d received (not RC5, ignored)", data.protocol);
    }

    IrReceiver.resume();
  }

  void pollEncoder()
  {
    bool encoderA = digitalRead(kEncoderPinA);
    if (encoderA != encoderLastA)
    {
      if ((millis() - encoderLastMoveMs) >= kEncoderDebounceMs && encoderA == LOW)
      {
        bool encoderB = digitalRead(kEncoderPinB);
        if (encoderB == HIGH)
        {
          debugLog("ENCODER: CW (volume up)");
          changeVolume(+1);
        }
        else
        {
          debugLog("ENCODER: CCW (volume down)");
          changeVolume(-1);
        }
        encoderLastMoveMs = millis();
      }
      encoderLastA = encoderA;
    }
  }

  void drawUi()
  {
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

  void pollButtons()
  {
    if (updateButton(encoderButton))
    {
      debugLog("BUTTON: encoder push detected (mute toggle)");
      toggleMute();
    }

    if (updateButton(sourceButton))
    {
      debugLog("BUTTON: source button detected (next source)");
      nextSource();
    }
  }

} // namespace

void setup()
{
  Serial.begin(115200);
  delay(100);
  debugLog("==================================================");
  debugLog("ESP32 Audio Controller BOOTING");
  debugLog("==================================================");
  debugLog("DEBUG output enabled: %s", kDebugEnabled ? "YES" : "NO");
  debugLog("Sources: %u", kSourceCount);
  debugLog("Relay pins: D2-D9");
  debugLog("IR pin: D%d", kIrPin);
  debugLog("Encoder: D%d(A), A%d(B), A%d(push)", kEncoderPinA, kEncoderPinB, kEncoderButtonPin);
  debugLog("Source button: A%d", kSourceButtonPin);
  debugLog("TFT: A%d(CS), A%d(DC)", kTftCsPin, kTftDcPin);
  debugLog("MAS6116 CS: A%d at 1MHz SPI", kMasCsPin);
  debugLog("--------------------------------------------------");

  pinMode(kMasCsPin, OUTPUT);
  digitalWrite(kMasCsPin, HIGH);

  SPI.begin(kSpiSckPin, -1, kSpiMosiPin);

  for (uint8_t index = 0; index < kSourceCount; ++index)
  {
    pinMode(kRelayPins[index], OUTPUT);
    digitalWrite(kRelayPins[index], kRelayOffLevel);
  }

  pinMode(kEncoderPinA, INPUT_PULLUP);
  pinMode(kEncoderPinB, INPUT_PULLUP);
  pinMode(kEncoderButtonPin, INPUT_PULLUP);
  pinMode(kSourceButtonPin, INPUT_PULLUP);
  pinMode(kIrPin, INPUT_PULLUP);

  preferences.begin("audioctl", false);
  debugLog("Preferences: initialized NVS 'audioctl'");
  loadState();

  debugLog("TFT: initializing display...");
  tft.begin();
  tft.setRotation(kDisplayRotation);
  tft.fillScreen(ILI9341_BLACK);
  debugLog("TFT: display initialized");

  debugLog("IR: initializing RC5 receiver...");
  IrReceiver.begin(kIrPin, DISABLE_LED_FEEDBACK);
  debugLog("IR: receiver ready on D%d", kIrPin);

  debugLog("Initializing audio path...");
  setRelaySource(currentSource);
  if (muted)
  {
    debugLog("SETUP: applying mute (-70 dB)");
    writeMasVolume(kMuteCode);
  }
  else
  {
    writeMasVolume(currentVolume);
  }

  debugLog("MAS6116: verifying initialization...");
  delay(10);
  uint8_t leftRead = readMasVolume(kMasLeftReg);
  uint8_t rightRead = readMasVolume(kMasRightReg);
  uint8_t expectedCode = muted ? kMuteCode : currentVolume;

  if (leftRead == expectedCode && rightRead == expectedCode)
  {
    debugLog("MAS6116: VERIFIED - Left=%3u, Right=%3u (expected=%3u)", leftRead, rightRead, expectedCode);
  }
  else
  {
    debugLog("MAS6116: ERROR - Left=%3u, Right=%3u (expected=%3u) - CHIP MAY BE OFFLINE", leftRead, rightRead, expectedCode);
  }

  uiDirty = true;
  drawUi();
  uiDirty = false;

  debugLog("==================================================");
  debugLog("BOOT COMPLETE - System ready");
  debugLog("==================================================");
  delay(500);
}

void loop()
{
  pollIrReceiver();
  pollEncoder();
  pollButtons();

  if (uiDirty)
  {
    drawUi();
    uiDirty = false;
  }
}