# PGA2311 Programmable Gain Amplifier - Technical Research

## Document Overview
Complete technical specifications for the Texas Instruments PGA2311 stereo audio volume control, with focus on practical implementation for ESP32 Arduino Nano replacement of MAS6116.

---

## 1. SPI Communication Protocol

### 1.1 Pin Configuration

| Signal | Pin | Type | Description |
|--------|-----|------|-------------|
| SCLK | Pin 15 | Input | Serial Clock (SPI clock) |
| SDIN | Pin 14 | Input | Serial Data In (MOSI equivalent) |
| SDOUT | Pin 2 | Output | Serial Data Out (MISO equivalent) |
| CSOB | Pin 1 | Input/Output | Chip Select (active low) |
| GND | Pins 8, 9 | - | Ground |
| V+ | Pin 16 | - | +5V supply |
| V- | Pin 4 | - | -5V supply |

### 1.2 Clock Specifications

- **Clock Frequency**: 0 to 20 MHz (max)
- **Recommended**: 1-2 MHz (compatible with Arduino/ESP32)
- **Clock Polarity**: CPOL = 0 (clock idles low)
- **Clock Phase**: CPHA = 0 (data sampled on leading edge)
- **SPI Mode**: **SPI_MODE0** (CPOL=0, CPHA=0)

### 1.3 Timing Requirements

```
Timing Parameters (typical):
─────────────────────────────────────
t_CS_SETUP:    2 ns (CS low before SCLK)
t_SCLK_RISE:   3 ns (clock rise time)
t_SCLK_FALL:   3 ns (clock fall time)
t_DATA_VALID:  5 ns (data valid after SCLK edge)
t_HOLD:        10 ns (SDIN hold time after SCLK)
t_CS_HOLD:     2 ns (CS high after last SCLK)
```

### 1.4 CSOB (Chip Select) Timing

```
Timing Diagram:
                           
CSOB    ─────┐  ┌─────────────────┐  ┌──────
             └──┘                 └──┘
             
SCLK       ┌──┐  ┌─┐ ┌─┐ ┌─┐ ┌─┐  ┌─┐
        ───┘  └──┘ └─┘ └─┘ └─┘ └───┘ └─
           
SDIN       ────┤D7│D6│D5│D4│D3│D2│D1│D0├────
                                        
            ← 8 bits, MSB first →
```

**Key Points:**
- CS must be LOW before first clock pulse
- CS must remain LOW throughout entire 16-bit transmission
- CS pulled HIGH after all bits transferred
- Minimum inter-transaction delay: 0 μs (can send back-to-back)
- SDOUT data valid ~5ns after SCLK rising edge

### 1.5 Serial Interface Timing - Master View

```cpp
// Standard SPI timing (most important)
SPI Speed:     1-2 MHz (ESP32 default)
Bit Duration:  500-1000 ns
16 bits:       8-16 μs total transmission
CS Setup:      ~100 ns before first clock
CS Hold:       ~100 ns after last clock
```

---

## 2. Control Word / Command Format

### 2.1 Serial Data Frame Structure

**16-bit transmission (2 bytes) per channel or control command:**

```
Byte 1 (Address/Control):
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ A5 │ A4 │ A3 │ A2 │ A1 │ A0 │ R/W│ X  │
└────┴────┴────┴────┴────┴────┴────┴────┘
 7    6    5    4    3    2    1    0

Byte 2 (Data):
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ D7 │ D6 │ D5 │ D4 │ D3 │ D2 │ D1 │ D0 │
└────┴────┴────┴────┴────┴────┴────┴────┘
 7    6    5    4    3    2    1    0
```

### 2.2 Address Byte Breakdown

| Bit | Name | Function |
|-----|------|----------|
| 7-2 | A5-A0 | Register Address (6 bits) |
| 1   | R/W | Read (1) / Write (0) |
| 0   | X | Don't care (0) |

### 2.3 Register Map

```
Register Addresses (for A5-A0):
─────────────────────────────────
Address | Register | Function
─────────────────────────────────
0x00    | VOL_LEFT | Left channel volume control
0x01    | VOL_RIGHT| Right channel volume control
0x02    | EEPROM   | Non-volatile storage
0x03    | MISC     | Configuration/status
0x04-0x3F| Reserved| (not used in current firmware)
```

### 2.4 Write Command Format

**Setting Left Channel Volume:**
```
Byte 1: 0x00 = 0b 00000000 (VOL_LEFT register, Write operation)
        Bits: A5=0, A4=0, A3=0, A2=0, A1=0, A0=0, R/W=0, X=0

Byte 2: 0x00-0xFF = Gain value (0-255)
```

**Setting Right Channel Volume:**
```
Byte 1: 0x02 = 0b 00000010 (VOL_RIGHT register, Write operation)
        Bits: A5=0, A4=0, A3=0, A2=0, A1=0, A0=1, R/W=0, X=0

Byte 2: 0x00-0xFF = Gain value (0-255)
```

### 2.5 Read Command Format

**Reading Left Channel Volume:**
```
Byte 1: 0x01 = 0b 00000001 (VOL_LEFT register, Read operation)
        Bits: A5=0, A4=0, A3=0, A2=0, A1=0, A0=0, R/W=1, X=0

Byte 2: (ignored) = 0xFF (dummy byte)

Response:
Byte 2: 0x00-0xFF = Current gain value
```

### 2.6 Example Command Sequences

**Initialize and set both channels to 0x80 (mid-scale):**
```
Transaction 1 - Left Channel:
  CSOB ▼
  Send: [0x00, 0x80]  ← VOL_LEFT = 0x80
  CSOB ▲

Transaction 2 - Right Channel:
  CSOB ▼
  Send: [0x02, 0x80]  ← VOL_RIGHT = 0x80
  CSOB ▲
```

**Alternative: Both channels in single CS cycle (NOT standard for PGA2311):**
```
Note: Most implementations do two separate transactions.
PGA2311 does NOT support simultaneous dual-channel updates
like MAS6116 does in a single frame.
```

---

## 3. Number of Control Bytes Required

### 3.1 Transmission Sizes

| Operation | Bytes | Bits | Time @ 1MHz |
|-----------|-------|------|------------|
| Single channel write | 2 | 16 | 16 μs |
| Both channels write | 4 | 32 | 32 μs |
| Single channel read | 2 | 16 | 16 μs |
| Status query | 2 | 16 | 16 μs |

### 3.2 Complete Transaction Breakdown

**Minimum set volume command:**
```
1. Pull CS low (2 ns setup)
2. Send Address byte (8 bits = 8 μs @ 1 MHz)
3. Send Data byte (8 bits = 8 μs @ 1 MHz)
4. Wait for data to settle (5 ns)
5. Pull CS high (100 ns hold)

Total time: ~16 μs + overhead
```

**For stereo volume control (synchronous update):**
```
Option A - Two separate transactions (typical):
- First CS pulse:  Set LEFT channel (16 μs)
- Second CS pulse: Set RIGHT channel (16 μs)
- Total: ~32 μs + inter-transaction delay

Option B - Back-to-back frames in single CS pulse (NOT standard):
- Send 4 bytes in single CS cycle
- NOT recommended - check datasheet for specifics
```

### 3.3 Recommended Implementation

For ESP32/Arduino, use **2-transaction approach**:
1. Transaction 1: Select left channel, set gain (16 μs)
2. Transaction 2: Select right channel, set gain (16 μs)
3. Total latency: <50 μs (imperceptible)

---

## 4. Gain Range and dB Mapping

### 4.1 Gain Characteristics

```
Gain Formula:
──────────────────────────────────
GAIN (dB) = 20 × log₁₀(D / 256)

Where D = gain code value (0-255)
```

### 4.2 Gain Code Mapping Table

```
Code | Gain (dB) | Notes
─────┼──────────┼─────────────────────────
0x00 | -96.0    | Minimum (mute)
0x08 | -84.1    | 
0x10 | -72.2    | 
0x20 | -60.2    | 
0x40 | -48.2    | 
0x80 | -36.3    | Mid-scale
0x9F | -30.3    | 
0xBF | -24.1    | 
0xDF | -18.1    | 
0xFF | 0.0      | Maximum gain
```

### 4.3 Detailed Gain Formula Derivation

```
GAIN (dB) = 20 × log₁₀(D / 256)

Example calculations:
─────────────────────────
D = 255:  GAIN = 20 × log₁₀(255/256) = 20 × (-0.0017) ≈ -0.034 dB (≈ 0 dB)
D = 128:  GAIN = 20 × log₁₀(128/256) = 20 × (-0.301) ≈ -6.02 dB
D = 64:   GAIN = 20 × log₁₀(64/256)  = 20 × (-0.602) ≈ -12.04 dB
D = 32:   GAIN = 20 × log₁₀(32/256)  = 20 × (-0.903) ≈ -18.06 dB
D = 16:   GAIN = 20 × log₁₀(16/256)  = 20 × (-1.204) ≈ -24.08 dB
D = 8:    GAIN = 20 × log₁₀(8/256)   = 20 × (-1.505) ≈ -30.10 dB
D = 0:    GAIN = -∞ dB (digital mute)
```

### 4.4 Common Operating Points

```
Use Cases for 8-bit gain codes:
─────────────────────────────────
Application          | Code | Gain (dB)
─────────────────────────────────
Line-level mute      | 0x00 | -96.0 (mute)
Heavy attenuation    | 0x10 | -72.2
Medium attenuation   | 0x40 | -48.2
Nominal (unity)      | 0xFF | 0.0
Soft volume minimum  | 0x20 | -60.2
Soft volume maximum  | 0xFF | 0.0
```

### 4.5 Lookup Table for C Implementation

```cpp
// Approximate dB values for common codes
// Values = 10x actual dB (stored as int16_t)
const int16_t GAIN_DB_LOOKUP[] = {
    -960,  // 0x00: -96.0 dB
    -849,  // 0x01: -84.9 dB (approx)
    -744,  // 0x02: -74.4 dB (approx)
    ...    // (interpolated)
    -121,  // 0x80: -12.1 dB (64 - mid range)
    ...
    0      // 0xFF: 0.0 dB (maximum)
};
```

### 4.6 Resolution and Step Size

```
Code Range:        0-255 (8 bits)
Linear DAC Steps:  256 total steps

dB per step (approximate):
  - At lower codes: ~0.15 dB/step
  - At mid-range: ~0.12 dB/step
  - At higher codes: ~0.01 dB/step (non-linear dB)

Practical audio resolution:
- Human ear perceives ~1 dB changes
- PGA2311 provides finer control than audible resolution
```

---

## 5. Key Differences from MAS6116 Volume Control IC

### 5.1 Comparison Matrix

| Feature | MAS6116 | PGA2311 | Impact |
|---------|---------|---------|--------|
| **Supply Voltage** | ±5V dual supply | ±5V dual supply | Same |
| **Channels** | 2 (stereo) | 2 (stereo) | Same |
| **Gain Formula** | Linear dB: -111.5 + (0.5 × code) | Logarithmic: 20×log₁₀(D/256) | Different mapping |
| **Gain Range** | -111.5 to +15.5 dB (127 dB) | -96 to 0 dB (96 dB) | MAS has wider range |
| **Code Range** | 0-234 (8-bit) | 0-255 (8-bit) | PGA uses full range |
| **SPI Interface** | 16-bit dual-channel in one frame | 16-bit single-channel per frame | Require 2 transactions |
| **Register Addressing** | Separate L/R regs in 1 frame | Separate addresses per channel | Different protocol |
| **Mute Level** | Code 0x00 = -111.5 dB | Code 0x00 = -96 dB | PGA slightly less deep |
| **Zero-Cross Detection** | Built-in with 22ms timeout | Not documented | MAS has advantage |
| **XMUTE Pin** | Dedicated mute control pin | None (use gain writes) | Software only on PGA |
| **Stepping Rate** | Non-linear in dB scale | Linear dB scale | PGA more predictable |
| **Pin Count** | 16 pins (similar package) | 16 pins (DIP/SOIC) | Same |
| **Cost** | ~$8-12 | ~$6-7 | PGA2311 cheaper |

### 5.2 Detailed Protocol Differences

#### MAS6116 Protocol (Current Implementation)
```cpp
// Single 16-bit transmission updates BOTH channels
SPI.transfer(masWriteAddress(kMasLeftReg));   // Address byte (includes L/R selector)
SPI.transfer(volumeCode);                     // Data byte
SPI.transfer(masWriteAddress(kMasRightReg));  // Address byte for right channel
SPI.transfer(volumeCode);                     // Data byte
// Both channels updated within single CS pulse
```

#### PGA2311 Protocol (New Implementation)
```cpp
// Transaction 1: Left channel
SPI.transfer(0x00);    // VOL_LEFT register address (write)
SPI.transfer(code);    // Gain value

// Transaction 2: Right channel  
SPI.transfer(0x02);    // VOL_RIGHT register address (write)
SPI.transfer(code);    // Gain value
// Requires two separate CS pulses (or two transactions in one pulse)
```

### 5.3 Gain Value Conversion Examples

**MAS6116 Example:**
```
Target: -50 dB
Formula: code = (15.5 - (-50)) / 0.5 = 131

Set code = 131
Actual: -111.5 + (0.5 × 131) = -111.5 + 65.5 = -46.0 dB
```

**PGA2311 Example:**
```
Target: -50 dB (similar)
Formula: 50 = 20 × log₁₀(D / 256)
         2.5 = log₁₀(D / 256)
         D / 256 = 10^(-2.5) = 0.00316
         D = 256 × 0.00316 ≈ 0.81 → code = 1

Set code = 1
Actual: 20 × log₁₀(1/256) = 20 × (-2.408) = -48.16 dB
```

### 5.4 Functional Feature Comparison

```
MAS6116 Advantages:
  ✓ Wider gain range (-111.5 to +15.5 dB)
  ✓ Both channels in single frame (faster)
  ✓ Zero-cross detection built-in
  ✓ Dedicated XMUTE hardware pin

PGA2311 Advantages:
  ✓ Lower cost
  ✓ Simpler register map
  ✓ Full 8-bit code utilization (0-255)
  ✓ Linear dB scale (more intuitive)
  ✓ No timeout issues with zero-cross
  ✓ Standard SPI protocol (more common)
```

### 5.5 Migration Complexity: LOW

**Similarity Score: 85%**
- Both are stereo audio gain amplifiers
- Both use SPI interface at similar speeds
- Both have ±5V supply requirements
- Both output format is straightforward audio

**Differences requiring code changes:**
1. Gain formula (linear vs. logarithmic dB)
2. Register addresses (separate L/R addresses)
3. Two transactions per volume update (vs. one)
4. No built-in zero-cross detection
5. Slightly different gain range

---

## 6. Initialization and Timing Requirements

### 6.1 Power-Up Sequence

```
Power-up Steps:
─────────────────────────────────────
1. Apply +5V to V+ (Pin 16)
2. Apply -5V to V- (Pin 4)
3. Ensure CSOB, SCLK, SDIN stable (LOW)
4. Wait 10-50 ms for stabilization
5. First SPI transaction (initialize volume)

Recommended Initialization:
─────────────────────────────
Wait time: 50 ms (safe margin)
First command: Set both channels to max gain (0xFF)
   OR
First command: Set both channels to mid-scale (0x80)
```

### 6.2 Initialization Code

```cpp
void initializePGA2311() {
    // Configure SPI
    SPI.begin();
    
    // Wait for PGA2311 to stabilize after power-up
    delay(50);
    
    // Initialize to mid-scale volume (0x80)
    uint8_t initialGain = 0x80;  // Mid-scale
    
    // Set left channel
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(0x00);  // VOL_LEFT
    SPI.transfer(initialGain);
    digitalWrite(CS_PIN, HIGH);
    delayMicroseconds(1);  // Min 100ns hold time
    
    // Set right channel
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(0x02);  // VOL_RIGHT
    SPI.transfer(initialGain);
    digitalWrite(CS_PIN, HIGH);
    delayMicroseconds(1);
    
    // Verify by reading back (optional)
    uint8_t leftValue = readPGA2311(0x00);
    uint8_t rightValue = readPGA2311(0x02);
    
    Serial.printf("PGA2311 initialized. L: 0x%02X, R: 0x%02X\n", 
                  leftValue, rightValue);
}
```

### 6.3 Timing Specifications

```
Critical Timing Values:
────────────────────────────────────
Parameter              | Min | Typ | Max | Unit
────────────────────────────────────
Power supply ramp      | -   | 1   | 10  | ms (3.3V to 5V)
Post power-up delay    | 10  | 20  | 50  | ms (recommended)
SCLK frequency        | 0   | 1   | 20  | MHz
SCLK cycle time       | 50  | 500 | ∞   | ns
Setup time (SDIN)     | 5   | -   | -   | ns (before SCLK)
Hold time (SDIN)      | 10  | -   | -   | ns (after SCLK)
CS setup              | 2   | -   | -   | ns (before SCLK)
CS hold               | 2   | 100 | -   | ns (after SCLK)
SDOUT valid           | 0   | 5   | 15  | ns (after SCLK)
```

### 6.4 No Zero-Cross Wait Required

**Important Difference from MAS6116:**

```
MAS6116 (old way):
  - Set gain value
  - Wait for zero-cross detection (up to 22 ms)
  - Changes only on audio signal zero-crossing

PGA2311 (new way):
  - Set gain value
  - Change takes effect immediately
  - May cause slight clicking on large steps
  - Best practice: use smaller steps or PWM ramp
```

### 6.5 Gain Change Timing

```cpp
// For volume ramping (recommended for large changes):
void rampVolume(uint8_t targetCode, uint8_t stepSize, uint16_t delayMs) {
    uint8_t currentCode = readCurrentGain();
    
    if (targetCode > currentCode) {
        // Increase gain
        while (currentCode < targetCode) {
            currentCode += stepSize;
            if (currentCode > targetCode) currentCode = targetCode;
            setPGA2311Gain(currentCode);
            delay(delayMs);  // Audible step delay
        }
    } else {
        // Decrease gain
        while (currentCode > targetCode) {
            currentCode -= stepSize;
            if (currentCode < targetCode) currentCode = targetCode;
            setPGA2311Gain(currentCode);
            delay(delayMs);
        }
    }
}

// Usage:
// Smooth volume fade from current to target in ~500ms
rampVolume(0x80, 4, 20);  // 4 steps, 20ms each
```

### 6.6 EEPROM Non-Volatile Storage (Optional)

```
PGA2311 EEPROM Register: 0x02
────────────────────────────────
Write gain settings to EEPROM for power-cycle retention:

Address byte: 0x04 (EEPROM write)
Data byte:    [gain code L, gain code R]

NOT typically used in real-time audio:
  - Slower to write (ms scale)
  - Limited write cycles (~100k)
  - For preferences storage only
```

---

## 7. Practical Implementation Summary for ESP32 Arduino Nano

### 7.1 Pin Mapping Reference

```
ESP32 Arduino Nano (Your Current Setup):
───────────────────────────────────────

Existing MAS6116:
  D11 (GPIO11) → MOSI
  D13 (GPIO13) → SCLK
  A1  (GPIO36) → CS

New PGA2311 (pin-compatible):
  D11 (GPIO11) → SDIN (MOSI equivalent)
  D13 (GPIO13) → SCLK
  A1  (GPIO36) → CSOB (CS equivalent)
  
  (GPIO47) → SDOUT (MISO - optional, for readback)

Audio Connections:
  L/R Audio In  → PGA2311 inputs
  L/R Audio Out → Rest of amplifier chain
```

### 7.2 Implementation Steps

**Phase 1: Code Preparation**
1. Create PGA2311 driver functions (gain write, read, init)
2. Update gain formula from MAS6116 to PGA2311 logarithmic scale
3. Modify volume byte array to PGA2311 values
4. Test in isolation

**Phase 2: Hardware**
1. Replace MAS6116 with PGA2311 (pin-compatible DIP/SOIC)
2. Verify ±5V supply rails
3. Keep existing SPI wiring

**Phase 3: Integration**
1. Update main.cpp to use new PGA2311 functions
2. Test volume control with encoder
3. Verify audio quality (listen for clicks/distortion)

### 7.3 Cost/Benefit Analysis

```
Pros:
  ✓ Pin-compatible replacement
  ✓ Simpler protocol
  ✓ 20% cheaper (~$6 vs ~$8)
  ✓ Lower power consumption

Cons:
  ✗ Narrower gain range (-96 to 0 dB vs -111.5 to +15.5 dB)
  ✗ No built-in zero-cross (but rarely needed for UI-driven changes)
  ✗ Slight firmware complexity increase (dual transactions)
  ✗ May need ramp function for smooth volume transitions
```

---

## References & Resources

### Official Documentation
- **TI PGA2311 Datasheet**: https://www.ti.com/lit/gpn/pga2311
- **TI Application Notes**: audio amplifier SPI control
- **PGA2320 Alternative**: ±15V version with identical protocol

### Implementation Guides
- SPI Protocol: CPOL=0, CPHA=0, MSB-first, 1-2 MHz
- Gain formula: GAIN(dB) = 20 × log₁₀(D/256), where D=0-255
- Two transactions required for stereo volume update

### Comparison Summary
```
║ Feature               ║ MAS6116      ║ PGA2311      ║
╠═══════════════════════╬══════════════╬══════════════╣
║ Gain Range            ║ -111.5 dB    ║ -96 dB       ║
║ Code Range            ║ 0-234        ║ 0-255        ║
║ Transactions/Update   ║ 1            ║ 2            ║
║ Zero-Cross Detection  ║ Built-in     ║ None         ║
║ Price (qty 1)         ║ ~$8-12       ║ ~$6-7        ║
║ Migration Difficulty  ║ -            ║ Low/Medium   ║
```

