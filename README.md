# ESP32 Audio Controller (Arduino Nano ESP32) with PGA2311

This project implements the full request in [request.md](request.md):

- Control 8 relay-selected audio sources
- Control a **PGA2311** stereo volume IC over SPI (logarithmic gain amplifier)
- Show source, volume, and mute status on a TFT display (HSD028309 class, ILI9341 SPI driver)
- Support IR remote control (Philips RC5 / RC5X-style command set)
- Support rotary encoder volume control and hardware buttons
- Mute behavior: force output to -96 dB, then restore original volume on unmute

## Request Completion Summary

All requested functional blocks are implemented in [src/main.cpp](src/main.cpp):

1. 8-source relay control: complete
2. PGA2311 volume control over SPI: complete
3. TFT display of source and volume number: complete
4. Mute/unmute with restore-to-previous-volume: complete
5. Mute attenuation target of -96 dB: complete (full chip mute)
6. Pin mapping per request: complete
7. IR command handling for amplifier-style control: complete

## Hardware Pin Mapping (Implemented)

### Relays

- D2 = Relay 1 = Phono
- D3 = Relay 2 = CD
- D4 = Relay 3 = Aux 1
- D5 = Relay 4 = Aux 2
- D6 = Relay 5 = DVD
- D7 = Relay 6 = Tuner
- D8 = Relay 7 = Tape 1
- D9 = Relay 8 = Tape 2
- Relay drive mode for ULN2003 is inverted/active-low:
  - selected input relay pin = LOW
  - all non-selected relay pins = HIGH

### Infrared Input

- D10 = IR receiver

### Shared SPI

- D11 = COPI/MOSI -> PGA2311 DATA + TFT SID
- D13 = SCK -> PGA2311 CCLK + TFT CLK

### PGA2311

- A1 = XCS (chip select)

### Controls

- D12 = Encoder A
- A3 = Encoder B
- A0 = Encoder push switch
- A2 = Source button

### TFT

- A4 = TFT CS
- A5 = TFT DC
- TFT RST = board RESET (`kTftRstPin = -1`, reset tied to board reset)
- TFT VCC = 3V3
- TFT LED = 3V3 or transistor-switched 3.3V

## Implemented Functional Behavior

### Source Selection

- 8 named inputs are defined:
  - Phono, CD, Aux 1, Aux 2, DVD, Tuner, Tape 1, Tape 2
- Only one relay is active at a time.
- Source can be changed by:
  - IR command
  - Front source button

### Volume Control (PGA2311)

- PGA2311 left and right channels are updated together over SPI in separate transactions.
- Volume code range is constrained to `0..255` (full 8-bit range).
- The displayed dB value is computed from the code using logarithmic formula.

### PGA2311 General Description (Datasheet-Based)

- PGA2311 is a stereo programmable gain amplifier for analog audio channels.
- Left and right channels can be programmed independently via separate SPI transactions.
- Gain range per channel is from -96.0 dB to 0.0 dB in logarithmic steps.
- Code 0x00 is the chip mute value (-96.0 dB maximum attenuation).
- Device supply is single +5 V while analog signal handling supports high-level input signals.
- No zero-cross detection required (immediate effect on volume changes).

### Gain Code Mapping (PGA2311)

- Gain is mapped logarithmically as:
  - Gain(dB) = 20 × log₁₀(code / 256)
- Examples:
  - code 0x00 = -96.0 dB (mute)
  - code 0x80 = -36.3 dB (mid-scale)
  - code 0xFF = 0.0 dB (maximum)

### Why This Project Uses Code 0x00 for Mute Behavior

- Your requested behavior is mute to -96 dB (full chip mute).
- In this firmware, mute is implemented by writing gain code 0x00 to both channels.
- This gives:
  - 20 × log₁₀(0x00 / 256) = -96.0 dB
- On unmute, the previously active per-source volume is restored exactly.

### Serial Interface Details (PGA2311)

- PGA2311 serial control uses:
  - DATA (MOSI for write, MISO for read)
  - XCS (chip select)
  - CCLK (clock)
- Register addresses (8-bit):
  - 0x00 = Left channel write
  - 0x01 = Left channel read
  - 0x02 = Right channel write
  - 0x03 = Right channel read
- A full register operation is 16 clock pulses while XCS is low:
  - first byte: address byte
  - second byte: gain value (0x00 to 0xFF)
- **Important**: Left and right channels must be written in separate SPI transactions.
- For stereo updates:
  - Transaction 1: Write left channel (address 0x00, gain value)
  - 1ms delay
  - Transaction 2: Write right channel (address 0x02, gain value)
- Shared bus behavior:
  - DATA and CCLK can be common to multiple devices
  - each PGA2311 responds only when its XCS is active

### Zero-Cross and Click Behavior (PGA2311)

- PGA2311 does not have built-in zero-cross detection like some older volume ICs.
- Volume changes take effect immediately, which may cause audible clicks on large step changes.
- In practice, user-controlled incremental volume changes (encoder/IR +/- commands) are small enough that clicks are not perceptible.
- For large jumps or source switching with different volumes, future versions could implement software ramp-up/ramp-down.

### Control Architecture (PGA2311)

- PGA2311 is a programmable gain amplifier with full SPI serial control.
- Unlike older mute-pin designs, all control is via SPI register writes.
- No dedicated mute or control pins required (only CS, CLK, DATA lines).
- Project approach:
  - Mute is implemented by writing 0x00 (-96 dB) to both channels via SPI
  - Volume control is implemented by writing gain codes (0x01–0xFF) to both channels
  - All state changes are immediate and synchronous

### Operating Modes and Power-Up Notes

- On power-up, PGA2311 requires a 50ms delay before first SPI operations.
- Channel activation in normal operation requires valid gain values per datasheet.
- In this firmware, startup sequence is:
  - load persisted state
  - select active relay source
  - write active volume (or mute code when muted state is persisted)

### PGA2311 Initialization Verification on Boot

- This firmware verifies PGA2311 register response during boot after initial volume write.
- Added helper functions:
  - `readPgaVolume(regAddr)`: performs SPI readback of a selected PGA2311 register.
- Verification flow in `setup()`:
  - Write startup volume to both channels (separate left/right transactions).
  - Wait **50 ms** for device power-up settling (PGA2311 requirement).
  - Read back left (address 0x01) and right (address 0x03) gain registers.
  - Compare both read values against expected startup code.
  
**Important**: PGA2311 requires a 50ms delay after power-up before first SPI operations. This is handled automatically in the boot sequence.

Example boot log when verification passes:

```text
[time] PGA2311: verifying initialization...
[time] PGA2311: VERIFIED - Left=180, Right=180 (expected=180)
```

Example boot log when verification fails:

```text
[time] PGA2311: ERROR - Left=255, Right=255 (expected=180) - CHIP MAY BE OFFLINE
```

If readback values do not match expected startup volume, serial logs clearly indicate a potential wiring, power, or chip-communication issue.

### PGA2311 Features (Not Currently Implemented)

- Datasheet includes advanced features not needed for basic volume control:
  - Buffered outputs option
  - Output impedance specifications
  - Input/output headroom planning
- Current firmware scope:
  - Implements source selection and volume control only
  - Does not use advanced input/output configurations

### Register and Command Summary (PGA2311)

- Gain registers (8-bit gain values, 0x00–0xFF):
  - 0x00 = Left channel write address
  - 0x01 = Left channel read address
  - 0x02 = Right channel write address
  - 0x03 = Right channel read address
- Data format:
  - Gain byte: 0x00 = -96 dB, 0x80 ≈ -36 dB, 0xFF = 0 dB (logarithmic scale)
- This project writes left and right channel gains as separate SPI transactions (1ms apart).

### Mute/Unmute

- On mute:
  - Current volume is stored (`muteRestoreVolume`)
  - PGA2311 is set to code `0x00` (target `-96.0 dB` full chip mute)
- On unmute:
  - Previous volume is restored
  - Audio returns to exactly the pre-mute level

### UI Display

The TFT view shows:

- Current source number (`1/8` to `8/8`)
- Current source name
- Current volume code (`Vol: N`)
- Current dB value (or `-70.0 dB` while muted)
- Status label: `MUTED` or `LIVE`

### Encoder and Buttons

- Rotary encoder adjusts volume up/down.
- Encoder push button toggles mute.
- Source button advances to next source.
- Debouncing is implemented for encoder and buttons.

### IR Remote

- IR receiver is configured on D10.
- RC5 protocol decoder path is enabled.
- Implemented command behavior:
  - `1..8`: direct source select
  - `13`: mute toggle
  - `16`: volume up
  - `17`: volume down
  - `26`: previous source
  - `27`: next source
- Address filter supports common amplifier and CD-style address values (`0x10`, `0x14`).

## Persistence (NVS Preferences)

The following state is saved and restored on reboot:

- Per-source volumes (all 8 inputs)
- Current source index
- Mute state
- Last pre-mute restore volume

This ensures the unit resumes prior operating state after power cycle.

## PlatformIO Configuration

Configured in [platformio.ini](platformio.ini):

- Board: `arduino_nano_esp32`
- Framework: Arduino
- Monitor speed: `115200`
- Libraries:
  - `adafruit/Adafruit GFX Library`
  - `adafruit/Adafruit ILI9341`
  - `z3t0/IRremote`

## Debug Tests

Quick per-component Arduino-format debug sketches are included to test each piece of hardware before running the main firmware. Each test is a self-contained `.ino` sketch in its own folder under `debug_tests/`:

- [debug_tests/i2c/i2c_test.ino](debug_tests/i2c/i2c_test.ino) — **Relay Test** (D2–D9): Toggles all 8 relays one at a time (Phono, CD, Aux1, Aux2, DVD, Tuner, Tape1, Tape2).
- [debug_tests/pga2311/pga2311_test.ino](debug_tests/pga2311/pga2311_test.ino) — **IR Remote Test** (D10): Receives and decodes RC5/RC5X IR commands.
- [debug_tests/pga2311_spi/pga2311_spi_test.ino](debug_tests/pga2311_spi/pga2311_spi_test.ino) — **PGA2311 SPI Test** (A1=XCS, D11=DATA, D13=CLK): Verifies SPI communication with chip-select toggles.
- [debug_tests/audio_output/audio_output_test.ino](debug_tests/audio_output/audio_output_test.ino) — **TFT Display Test** (A4=CS, A5=DC, D11=SID, D13=CLK): Initializes and draws colored rectangles on the HSD028309 TFT.
- [debug_tests/wifi/wifi_test.ino](debug_tests/wifi/wifi_test.ino) — **Encoder & Buttons Test** (D12=EncoderA, A3=EncoderB, A0=EncoderPush, A2=SourceBtn): Monitors rotary encoder and button presses.

### Running a Debug Test in Arduino IDE

1. Open Arduino IDE 2.x.
2. Open one of the debug sketches (e.g., `debug_tests/i2c/i2c_test.ino`).
3. Select board: **Arduino Nano ESP32**.
4. Select the correct **COM port** for your device.
5. Click **Verify**, then **Upload**.
6. Open **Serial Monitor** at `115200` baud to see test results.

### Notes

- All pin assignments in the debug sketches are hard-coded to match the pinout in [request.md](request.md); edit them only if your wiring differs.
- Older `.cpp`/`.h` test files remain in `debug_tests/` root for reference; remove them if you prefer a single canonical set of sketches.
- These debug sketches are designed for quick hardware verification only; they halt in `setup()` after running the test or enter an idle loop.


## Arduino IDE Setup and Upload (Client Workflow)

This project can be used directly in Arduino IDE 2.x for clients who do not use PlatformIO.

### 1. Install Arduino IDE and Board Support

- Install Arduino IDE 2.x.
- Open Arduino IDE -> Board Manager.
- Install ESP32 board support that includes Arduino Nano ESP32.
- Select board: Arduino Nano ESP32.

### 2. Install Required Libraries (Library Manager)

- Adafruit GFX Library
- Adafruit ILI9341
- IRremote (version compatible with `#include <IRremote.hpp>` and RC5 decode)

`Preferences.h` and `SPI.h` are part of the ESP32 Arduino core and do not require separate installation.

### 3. Import Firmware into Arduino IDE

- Open the `main/main.ino` sketch in Arduino IDE.
- If you prefer, copy `main/main.ino` into a new sketch folder (example: `esp32_audio`).
- Keep all pin mappings and constants unchanged unless your hardware wiring differs.

### 4. Verify Critical Relay Logic for ULN2003

For ULN2003 relay boards, control is inverted and this firmware already matches that requirement:

- active source relay output is LOW
- all inactive source relay outputs are HIGH

In code this is defined as:

- `kRelayOnLevel = LOW`
- `kRelayOffLevel = HIGH`

This ensures all not-enabled inputs stay high, and only the selected input is driven low.

### 5. Compile and Upload

- Connect Arduino Nano ESP32 over USB.
- Choose the correct COM port in Arduino IDE.
- Click Verify, then Upload.

### 6. Serial Monitor (Debug)

- Open Serial Monitor at 115200 baud.
- On boot you should see startup logs, IR command logs, and PGA2311 initialization verification messages.

## Source of Truth

- Request text: [request.md](request.md)
- Arduino implementation: [main/main.ino](main/main.ino)
- Build config: [platformio.ini](platformio.ini)

## Notes

- The HSD028309 panel is handled using the ILI9341 SPI driver stack, which matches common 2.8 inch TFT modules in this class.
- Relay logic is configured as active-low (`kRelayOnLevel = LOW`, `kRelayOffLevel = HIGH`) to support ULN2003-style inverted relay drive.
- If a different relay board requires active-high control, swap those constants.
- PGA2311 requires 50ms power-up delay before first SPI operations (handled in setup()).
- Gain code mapping for PGA2311 is logarithmic (unlike older linear volume ICs).
