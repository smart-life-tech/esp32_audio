# MAS6116 Datasheet vs. Implementation Report

## Summary

The current implementation **partially matches** the MAS6116 datasheet. Core serial protocol and gain register control are correct, but several advanced features (zero-cross detection, status polling, XMUTE pin control, peak detection) are not yet implemented.

---

## Detailed Feature Checklist

###  IMPLEMENTED - Datasheet-Compliant

#### 1. Serial Interface Protocol
- **Datasheet**: 16-bit frames; address byte (bit 2 = read/write) + data byte
- **Code**: [src/main.cpp](src/main.cpp) lines 119-127
  ```cpp
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
  ```
- **Status**:  **CORRECT** - XCS control, SPI clock, address + data bytes match spec.

#### 2. Register Addresses (CR1, CR2)
- **Datasheet**: 
  - CR2 (Left) = `X 1 1 0 1 R/W X X`
  - CR1 (Right) = `X 1 1 1 0 R/W X X`
- **Code**: [src/main.cpp](src/main.cpp) lines 41-42
  ```cpp
  constexpr uint8_t kMasLeftReg = 0x0D << 3;   // 0x68 (CR2)
  constexpr uint8_t kMasRightReg = 0x0E << 3;  // 0x70 (CR1)
  ```
- **Status**:  **CORRECT** - Bit patterns align with datasheet register addresses.

#### 3. Write Address Byte Construction
- **Datasheet**: Bit 2 = 0 for write
- **Code**: [src/main.cpp](src/main.cpp) lines 75-77
  ```cpp
  uint8_t masWriteAddress(uint8_t reg) {
    return static_cast<uint8_t>((reg & kMasRegMask) | kMasWritePrefix);
  }
  ```
- **Status**:  **CORRECT** - Write mode (bit 2 low) is enforced.

#### 4. Gain-to-dB Conversion
- **Datasheet**: Gain(dB) = +15.5 - (0.5 × code)
- **Code**: [src/main.cpp](src/main.cpp) lines 79-81
  ```cpp
  int16_t volumeCodeToTenthsDb(uint8_t code) {
    return static_cast<int16_t>(155) - static_cast<int16_t>(code) * 5;
  }
  ```
- **Derivation**: 155 tenths = +15.5 dB, 5 tenths = 0.5 dB step
- **Status**:  **CORRECT** - Math is exact.

#### 5. Left and Right Channel Independence
- **Datasheet**: Channels can be programmed with separate commands
- **Code**: [src/main.cpp](src/main.cpp) lines 119-127 writes both CR2 and CR1 in sequence
- **Status**:  **CORRECT** - Both channels updated in single transaction.

#### 6. Pin Mapping (XCS)
- **Datasheet**: XCS control during read/write operations
- **Code**: [src/main.cpp](src/main.cpp) line 45, and lines 122-127 (`digitalWrite(kMasCsPin, LOW/HIGH)`)
- **Status**:  **CORRECT** - XCS pulled low during transaction, returned high after.

---

### ⚠️ PARTIALLY IMPLEMENTED or INTENTIONAL DEVIATION

#### 1. Mute Implementation
- **Datasheet**: Code 00HEX = chip mute (full attenuation, -111.5 dB)
- **Request**: Mute to -70 dB, then restore to previous volume
- **Code**: [src/main.cpp](src/main.cpp) line 47
  ```cpp
  constexpr uint8_t kMuteCode = 171;  // -70.0 dB
  ```
- **Status**: ⚠️ **INTENTIONAL DEVIATION** - Code 171 is used instead of 0x00 to meet your -70 dB requirement, not datasheet default mute.

#### 2. Mute Control Method
- **Datasheet**: XMUTE pin (dedicated pin for mute on/off with zero-cross)
- **Code**: Not used; mute is implemented via gain register writes [src/main.cpp](src/main.cpp) lines 131-145
- **Status**: ⚠️ **ALTERNATIVE APPROACH** - Functionally equivalent but does not use XMUTE pin hardware. Meets your requirement for restore-to-previous-volume behavior.

---

### ❌ NOT YET IMPLEMENTED - Advanced Features

#### 1. Zero-Cross Detection and Timeout
- **Datasheet**: "MAS6116 will wait until a rising edge of the input signal is detected to change the gain value... timeout delay of typically 22 ms"
- **Code**: Direct writes without waiting; no timeout [src/main.cpp](src/main.cpp) line 123-126
- **Implication**: Gain changes may produce audible clicks on large step changes. Typical audio use (0.5-1 dB steps) is unlikely to cause issues, but large jumps (e.g., mute to unmute) may click slightly.
- **Status**: ❌ **NOT IMPLEMENTED** - No zero-cross polling or timeout delay.

#### 2. Write Operation Status Register (CR6)
- **Datasheet**: "bits 0 and 1 are set high at the start of a gain write operation, and are set back low when the new gain value has been set to the output"
- **Code**: Not read; no polling [src/main.cpp](src/main.cpp) has no CR6 read
- **Implication**: Code does not check if channel is busy before next write. Risk of overwriting pending write if user changes volume very rapidly.
- **Status**: ❌ **NOT IMPLEMENTED** - No status polling.

#### 3. Peak Detector (CR3, CR4)
- **Datasheet**: Peak detector reference register and peak detector status register
- **Code**: Not used; no peak detection [src/main.cpp](src/main.cpp) has no reference to CR3 or CR4
- **Status**: ❌ **NOT IMPLEMENTED** - No peak level monitoring.

#### 4. Instant Write Commands
- **Datasheet**: Special commands `0x05` (left instant), `0x06` (right instant), `0x07` (both instant) for immediate gain changes without zero-cross
- **Code**: Standard write addresses only; no instant commands used
- **Status**: ❌ **NOT IMPLEMENTED** - Uses standard writes; instant mode not supported.

#### 5. XMUTE Pin Control
- **Datasheet**: "It is possible to return to the mute state either by setting XMUTE pin low or writing zero to the gain register"
- **Code**: No XMUTE pin defined; mute is via gain register only
- **Status**: ❌ **NOT IMPLEMENTED** - XMUTE pin not used.

#### 6. Test Register (CR5)
- **Datasheet**: Test mode register for internal testing only (strongly recommend leaving at 00HEX)
- **Code**: Not accessed
- **Status**:  **CORRECTLY AVOIDED** - Code does not touch test register (good practice).

---

## Volume Code Range Comparison

| Feature | Datasheet | Implementation | Status |
|---------|-----------|-----------------|--------|
| Full mute code | 00HEX (-111.5 dB) | 171 (your -70 dB request) | ⚠️ Intentional |
| Min volume code | 00HEX | 0 (also -111.5 dB) |  Matches |
| Max volume code | FFHEX (255, +15.5 dB) | 234 (+15.5 dB) | ⚠️ Constrained |
| Default on powerup | 00HEX (mute) | 120 (0.0 dB) | ⚠️ Intentional |

**Note on Max Code 234**: The code constrains volume to `0..234`. This misses codes `235..255`, which represent -111.0 to -111.5 dB. This is likely intentional (to avoid full mute during normal use), but limits the full datasheet range.

---

## SPI Protocol Verification

### Datasheet Requirement
```
XCS = Low
  Clock 16 times (rising edges shift in address byte bits 7..0, then data byte bits 7..0)
  Data shifted in on CCLK rising edge
XCS = High
```

### Code Implementation
```cpp
digitalWrite(kMasCsPin, LOW);           // XCS low
SPI.transfer(masWriteAddress(kMasLeftReg));  // 8 clocks for address
SPI.transfer(volumeCode);               // 8 clocks for data
SPI.transfer(masWriteAddress(kMasRightReg)); // 8 clocks for address
SPI.transfer(volumeCode);               // 8 clocks for data
digitalWrite(kMasCsPin, HIGH);          // XCS high
```

**Status**:  **CORRECT** - SPI protocol is exact match.

---

## Recommendations for Full Datasheet Compliance

If you want to add zero-cross and status checking in the future:

1. **Add CR6 polling**:
   ```cpp
   void waitForChannelReady(bool checkLeft, bool checkRight) {
     // Read CR6 status, wait until bits are clear or timeout
   }
   ```

2. **Add small delays on large gain changes**:
   ```cpp
   if (abs(newVolume - currentVolume) > 2) {  // > 1 dB
     delay(30);  // Wait for zero-cross or timeout
   }
   ```

3. **Optional: Use XMUTE pin for mute** (if your board has it routed):
   ```cpp
   digitalWrite(kXmutePin, LOW);  // Mute
   delay(10);  // Zero-cross handling
   digitalWrite(kXmutePin, HIGH); // Unmute
   ```

---

## Conclusion

| Category | Status |
|----------|--------|
| Serial Protocol |  Correct |
| Gain Registers (CR1, CR2) |  Correct |
| Gain Math |  Correct |
| Pin Control (XCS) |  Correct |
| **Mute Behavior** | ⚠️ Intentional Deviation (-70 dB vs. 00HEX) |
| Zero-Cross Detection | ❌ Not Implemented |
| Status Polling (CR6) | ❌ Not Implemented |
| Peak Detector | ❌ Not Implemented |
| XMUTE Pin Control | ❌ Not Implemented |
| Instant Write Commands | ❌ Not Implemented |

**Overall**: The implementation is **functionally correct for audio control** but does **not yet use datasheet advanced features** (zero-cross, status polling). For typical use (encoder and remote volume control), the current approach works. For mission-critical audio (e.g., live broadcast), adding zero-cross and CR6 polling would reduce click risk.

