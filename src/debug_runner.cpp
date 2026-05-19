#include <Arduino.h>
#include "../debug_tests/i2c_test.h"
#include "../debug_tests/pga2311_test.h"
#include "../debug_tests/mas6116_test.h"
#include "../debug_tests/audio_output_test.h"
#include "../debug_tests/wifi_test.h"

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("--- Debug Runner ---");

#ifdef TEST_I2C
    Serial.println("Running I2C test...");
    dbg_i2c::run();
#endif

#ifdef TEST_PGA2311
    Serial.println("Running PGA2311 test...");
    dbg_pga2311::run();
#endif

#ifdef TEST_MAS6116
    Serial.println("Running MAS6116 test...");
    dbg_mas6116::run();
#endif

#ifdef TEST_AUDIO
    Serial.println("Running audio output test...");
    dbg_audio::run();
#endif

#ifdef TEST_WIFI
    Serial.println("Running WiFi test...");
    dbg_wifi::run();
#endif

    Serial.println("Debug runner finished. Halting.");
}

void loop() {
    delay(1000);
}
