# ESP32 Audio Controller (Arduino Nano ESP32)

This project implements the full request in [request.md](request.md):

- Control 8 relay-selected audio sources
- Control a MAS6116 stereo volume IC over shared SPI
- Show source, volume, and mute status on a TFT display (HSD028309 class, ILI9341 SPI driver)
- Support IR remote control (Philips RC5 / RC5X-style command set)
- Support rotary encoder volume control and hardware buttons
- Mute behavior: force output to -70 dB, then restore original volume on unmute

## Request Completion Summary

All requested functional blocks are implemented in [src/main.cpp](src/main.cpp):

1. 8-source relay control: complete
2. MAS6116 volume control over SPI: complete
3. TFT display of source and volume number: complete
4. Mute/unmute with restore-to-previous-volume: complete
5. Mute attenuation target of -70 dB: complete (`kMuteCode = 171`)
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

### Infrared Input

- D10 = IR receiver

### Shared SPI

- D11 = COPI/MOSI -> MAS6116 DATA + TFT SID
- D13 = SCK -> MAS6116 CCLK + TFT CLK

### MAS6116

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

### Volume Control (MAS6116)

- MAS6116 left and right channels are updated together over SPI.
- Volume code range is constrained to `0..234`.
- The displayed dB value is computed from the code in 0.5 dB steps.

### MAS6116 General Description (Datasheet-Based)

- MAS6116 is a stereo digital volume controller for analog audio channels.
- Left and right channels can be programmed independently.
- Gain range per channel is from -111.5 dB to +15.5 dB in 0.5 dB steps.
- Code 00HEX is the chip mute value (maximum attenuation).
- Device supply is single +5 V while analog signal handling supports high-level input signals.

### Gain Code Mapping

- Gain is mapped as:
  - Gain(dB) = +15.5 - (0.5 x code)
- Examples:
  - code 255 -> +15.5 dB
  - code 224 -> 0.0 dB
  - code 2 -> -111.0 dB
  - code 1 -> -111.5 dB
  - code 0 -> Mute

### Why This Project Uses Code 171 for Mute Behavior

- Your requested behavior is mute to -70 dB, not full chip mute.
- In this firmware, mute is implemented by writing gain code 171 to both channels.
- This gives:
  - +15.5 - (0.5 x 171) = -70.0 dB
- On unmute, the previously active per-source volume is restored exactly.

### Serial Interface Details

- MAS6116 serial control uses:
  - DATA (bi-directional)
  - XCS (chip select)
  - CCLK (clock)
- A full register operation is 16 clock pulses while XCS is low:
  - first byte: address/control
  - second byte: data/control word
- For write operations in this project:
  - XCS goes low
  - address byte is shifted in
  - data byte is shifted in
  - XCS returns high after bit 16
- Shared bus behavior:
  - DATA and CCLK can be common to multiple devices
  - each MAS6116 responds only when its XCS is active

### Zero-Cross and Timeout Behavior

- Standard gain updates use zero-cross detection to reduce audible clicks.
- Timeout logic ensures updates still complete even with no useful input signal.
- Datasheet guidance indicates waiting for write completion or checking status before next update to avoid overwriting pending writes.
- This project performs direct writes for responsive user interaction and keeps step changes small during normal control.

### XMUTE Pin Behavior (Datasheet)

- Datasheet behavior:
  - XMUTE low mutes both channels.
  - XMUTE high returns to previously programmed gain values.
  - transitions use zero-cross and timeout handling internally.
- Project behavior:
  - this firmware does not use a dedicated XMUTE pin control path.
  - instead, gain registers are written directly to enforce -70 dB mute target and exact restore behavior.

### Operating Modes and Power-Up Notes

- On power-up, MAS6116 internal reset initializes control registers.
- Channel activation in normal operation requires valid gain values and mute release conditions per datasheet.
- In this firmware, startup sequence is:
  - load persisted state
  - select active relay source
  - write active volume (or mute code when muted state is persisted)

### MAS6116 Initialization Verification on Boot

- This firmware verifies MAS6116 register response during boot after initial volume write.
- Added helper functions:
  - `masReadAddress(reg)`: builds the MAS6116 read address byte (read bit set).
  - `readMasVolume(reg)`: performs SPI readback of a selected MAS6116 register.
- Verification flow in `setup()`:
  - Write startup volume (or mute code `171` when muted state is restored).
  - Wait 10 ms for device settling.
  - Read back left (CR2) and right (CR1) gain registers.
  - Compare both read values against expected startup code.

Example boot log when verification passes:

```text
[time] MAS6116: verifying initialization...
[time] MAS6116: VERIFIED - Left=120, Right=120 (expected=120)
```

Example boot log when verification fails:

```text
[time] MAS6116: ERROR - Left=255, Right=255 (expected=120) - CHIP MAY BE OFFLINE
```

If readback values do not match expected startup volume, serial logs clearly indicate a potential wiring, power, or chip-communication issue.

### Peak Detector and Status Registers

- Datasheet includes:
  - peak detector reference register (CR3)
  - peak detector status register (CR4)
  - write-operation status register (CR6)
- Current firmware scope:
  - implements source and volume control only
  - does not currently read peak detector or CR6 write status bits

### Register and Command Summary (Relevant to This Project)

- Gain registers:
  - CR2 Left gain
  - CR1 Right gain
- Address byte bit 2 selects read/write:
  - 1 = read
  - 0 = write
- This project writes left and right channel gains as separate writes in one transaction sequence.

### Test Register Caution

- MAS6116 test register CR5 is intended for internal test use only.
- Datasheet recommends keeping CR5 at default 00HEX in normal operation.
- This project does not modify test mode settings.

### Mute/Unmute

- On mute:
  - Current volume is stored (`muteRestoreVolume`)
  - MAS6116 is set to code `171` (target `-70.0 dB`)
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

## Source of Truth

- Request text: [request.md](request.md)
- Implementation: [src/main.cpp](src/main.cpp)
- Build config: [platformio.ini](platformio.ini)

## Notes

- The HSD028309 panel is handled using the ILI9341 SPI driver stack, which matches common 2.8 inch TFT modules in this class.
- Relay logic is currently configured as active-low (`kRelayOnLevel = LOW`, `kRelayOffLevel = HIGH`). If your relay board is active-high, swap those constants.
