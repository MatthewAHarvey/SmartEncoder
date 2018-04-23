#include <HardwareSerial.h>
#include "SEHandler.h"

SEHandler encoders(Serial3);
event_t event;

void setup()
{
    Serial.begin(115200);
    Serial.println("Connected to SEHandlerTest.ino");

    uint8_t nEncoders = encoders.init();
    if(nEncoders)
    {
        Serial.print("Number of encoders: ");
        Serial.println(nEncoders);
    }
    else
    {
        Serial.println("No Smart Encoders detected.");
    }
    if(encoders.setRate2Max(255, 75))
    {
        Serial.println("Updated setRate2Max."); // 255 is global address so sets all encoders. 75 is milliseconds to trigger rate 2 when turning the encoder.
    }
    else
    {
        Serial.println("setRate2Max Update Failed.");
    }

    Serial.println();
    Serial.print("getRate: "); Serial.println(encoders.getRate2Max(1));
    Serial.print("getRate: "); Serial.println(encoders.getRate2Max(2));
    Serial.print("getRate: "); Serial.println(encoders.getRate2Max(3));
    Serial.print("getRate: "); Serial.println(encoders.getRate2Max(4));
}

void loop()
{
    event = encoders.poll();
    if(event.address)
    {
        // if the address isn't 0 then there has been an event. This means addresses must start at 1 and end at 254. Can have 254 encoders which is more than enough. What would the delay be with that many?
        Serial.print("Encoder "); Serial.print(event.address);
        Serial.print(": "); //Serial.println(event.result);
        switch(event.result)
        {
            case NO_CHANGE:
                break;
            case CW_RATE1: 
                Serial.println("CW_RATE1");
                break;
            case CW_RATE2:
                Serial.println("CW_RATE2");
                break;
            case CW_RATE3: 
                Serial.println("CW_RATE3");
                break;
            case ACW_RATE1: 
                Serial.println("ACW_RATE1");
                break;
            case ACW_RATE2: 
                Serial.println("ACW_RATE2");
                break;
            case ACW_RATE3: 
                Serial.println("ACW_RATE3");
                break;
            case BUTTON_UP: 
                Serial.println("BUTTON_UP");
                break;
            case BUTTON_DOWN: 
                Serial.println("BUTTON_DOWN");
                break;
            case BUTTON_DOUBLECLICK: 
                Serial.println("BUTTON_DOUBLECLICK");
                break;
            case BUTTON_HOLD:
                Serial.println("BUTTON_HOLD");
                break;
        }
    }
}