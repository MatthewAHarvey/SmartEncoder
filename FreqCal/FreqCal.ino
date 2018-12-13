//Program to calibrate the internal oscillator of an ATTINY85
//Once the fuses have been set to use the PLL at 16MHz, run this program and watch output on scope/frequency counter.
//Use Timer1 in CTC
//According to oscilloscope, this is spot on 1MHz. Should mean software serial works.

//Added a rotary encoder. Watch the frequency on the scope and change it with the encoder. Press button to write to EEPROM.

#include <EEPROM.h>
#include "Encoder.h"

// define pins
const int encPinA = 4; //pin 2 
const int encPinB = 3; //pin 3 
const int encPinC = 2; //pin 7

Encoder enc(encPinA, encPinB, encPinC);


void setup() {
    // put your setup code here, to run once:
    DDRB |= (1 << DDB1); // set PB1 to output mode
    PORTB |= (1 << PORTB1); //set output of PB1 to 1 initially.
    TCNT1 = 0; //Initialise the Timer1 counter value to 0

    // TCCR1 is the Timer/Counter1 Control Register
    TCCR1 = 0; // set everything to 0 which is the default anyway.
    TCCR1 |= (1 << CTC1); // Set the bit7 to 1 so that the timer is reset on a match with OCR1C
    TCCR1 |= (1 << COM1A0); // if COM1A1 is zero and COM1A0 is one, the output will toggle when TCNT1 == OCR1C
    TCCR1 |= (1 << CS10); // set prescaler to 1. Without this, the counter is disabled.

    //OCR1C = 0; // when this is zero, the output frequency should be f = 16M/(2*P*(1+OCR1C) = 8MHz where P is the prescaler factor
    OCR1C = 7; // when this is 7, the output frequency should be f = 16M/(2*P*(1+OCR1C) = 1MHz where P is the prescaler factor

    // Check for an existing calibration set in the EEPROM and if so, start from there.
    if(EEPROM.read(0) == 'C' && EEPROM.read(1) == 'A' && EEPROM.read(2) == 'L')
    {
        OSCCAL = EEPROM.read(3);
    }

    

    enc.init();
}

void loop() {
    // put your main code here, to run repeatedly:
    switch(enc.poll())
    {
        case NO_CHANGE:
            break;
        case CW_RATE1: 
              
        case CW_RATE2:
            
        case CW_RATE3: 
            OSCCAL++;
            break;
        case ACW_RATE1: 
            
        case ACW_RATE2: 
            
        case ACW_RATE3: 
            OSCCAL--;
            break;
        case BUTTON_UP: 
            EEPROM.update(0, 'C'); // Write CAL so the main code can check for this pattern. If it isn't present, it will not update OSCCAL.
            EEPROM.update(1, 'A');
            EEPROM.update(2, 'L');
            EEPROM.update(3, OSCCAL);
            break;
        case BUTTON_DOWN: 
            break;
        case BUTTON_DOUBLECLICK: 
            break;
        case BUTTON_HOLD:
            break;
    }
}