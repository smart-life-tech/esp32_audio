Quick debug tests for individual hardware components

Usage:
- Build a debug environment with PlatformIO, for example:
  `pio run -e debug_i2c -t upload` to run the I2C scanner test.
- Each debug environment sets a macro (e.g. `TEST_I2C`) and the `src_filter`
  so only `src/debug_runner.cpp` and files in `debug_tests/` are compiled.

Files:
- `i2c_test.*` - I2C bus scanner
- `pga2311_test.*` - simple I2C write probe for PGA2311 (address may need updating)
- `mas6116_test.*` - GPIO/toggle stub for MAS6116 control lines
- `audio_output_test.*` - DAC output tone generator using `dacWrite`
- `wifi_test.*` - quick WiFi AP bring-up to verify WiFi hardware

Notes:
- Edit pins/addresses inside the test files to match your wiring.
- These tests are intentionally small and non-invasive; modify as needed.
