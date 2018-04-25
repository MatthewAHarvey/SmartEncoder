# SmartEncoder
Arduino friendly, serially addressable, ATTINY85 based rotary encoders. Reduce the pin count from 3 * n where n is the number of encoders (pins A, B and button C) to 2 total. Serial in, Serial out. Use Hardware Serial, AltSoftSerial or Software Serial

Handling lots of encoders at the same time can be a challenge. For example, in our use case, we had 15 encoders attached to one Arduino Mega. After all the connections to other peripherals, we ran out of pins. The encoders had to be polled since there are not enough interrupts. Our solution is to offload the handling of the encoders to dedicated ATTINY85 chips. These then talk asynchronous serial at 125000 baud in a ring with the master arduino. The arduino now only needs 2 pins: 1 serial tx and 1 serial rx. At the moment this is limited to one of the hardware serial ports but tests show it's possible with software serial implementations.

In order for this to work, the ATTINY85s' clock frequencies must be set more accurately than straight from the factory. 

### Installation instructions:

1. First, the ATTINYs must be frequency calibrated. We do this by burning the arduino bootloader on to an ATTINY and uploading the FreqCal sketch. We output a square wave from pin 6 and observe it using an oscilloscope. An attached rotary encoder is then used to adjust this signal until it is as close to 1 MHz as possible. Pressing the encoder button saves the current OSCCAL register value to the EEPROM. 

Arduino IDE settings for ATTINY85:
![ATTINY85 Settings](/FreqCal/Images/ATTINY85_Settings.png | width=100)
