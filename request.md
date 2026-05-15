Create a arduino nano ESP32 program to control 8 relay audio sources and control a MAS6116 providing a lcd display for the 8 inputs volume control level and mute on and off functions. using a tft display HSD028309 so it shows source on display and volume number and when unmutes goes back to original volume level, mute goes to  -70db  Pin outs are as follows.

 IR remote receive (Philips RC5 / RC5X “extended standard preamp/amplifer commands

Relays
* D2 = Relay 1 = Phono
* D3 = Relay 2 = CD
* D4 = Relay 3 = Aux 1
* D5 = Relay 4 = Aux 2
* D6 = Relay 5 = DVD
* D7 = Relay 6 = Tuner
* D8 = Relay 7 = Tape 1
* D9 = Relay 8 = Tape 2
Infrared Input
* D10 = IR receiver 

Shared SPI
* D11 = COPI/MOSI → MAS6116 DATA + TFT SID
* D13 = SCK → MAS6116 CCLK + TFT CLK

MAS6116
* A1 = XCS
Controls
* D12 = Encoder A
* A3 = Encoder B
* A0 = Encoder push switch
* A2 = Source button
TFT
* A4 = TFT CS
* A5 = TFT DC
* TFT RST = board RESET
* TFT VCC = 3V3
* TFT LED = 3V3 or transistor-switched 3.3V