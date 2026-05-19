// IR Remote Receiver Test
// D10 = IR receiver input
#include <IRremote.hpp>

static const int IR_RECV_PIN = 10;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("=== IR REMOTE TEST ===");
  
  IrReceiver.begin(IR_RECV_PIN);
  Serial.println("IR receiver initialized on D10.");
  Serial.println("Point your RC5/RC5X remote at the receiver and press buttons...");
}

void loop() {
  if (IrReceiver.decode()) {
    Serial.print("Address: 0x");
    Serial.print(IrReceiver.decodedIRData.address, HEX);
    Serial.print(", Command: 0x");
    Serial.print(IrReceiver.decodedIRData.command, HEX);
    Serial.print(", Protocol: ");
    Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));
    
    IrReceiver.resume();
  }
  delay(100);
}
