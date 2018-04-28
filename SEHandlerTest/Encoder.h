#ifndef Encoder_h
#define Encoder_h

// A simple to use rotary encoder class. Place the poll function in an interrupt routine or main code. Must be checked very regularly if in main code or it WILL miss events.
// A helper function to perform actions based on encoder results
// void checkEncoder()
// {
//     if(state == RUNNING)
//     {
//         switch(enc.poll())
//         {
//             case NO_CHANGE:
//                 break;
//             case CW_RATE1: 
//                 
//                 break;
//             case CW_RATE2:
//                 
//                 break;
//             case CW_RATE3: 
//                 
//                 break;
//             case ACW_RATE1: 
//                 
//                 break;
//             case ACW_RATE2: 
//                 
//                 break;
//             case ACW_RATE3: 
//                 
//                 break;
//             case BUTTON_UP: 
//                 
//                 break;
//             case BUTTON_DOWN: 
//                 
//                 break;
//             case BUTTON_DOUBLECLICK: 
//                 
//                 break;
//             case BUTTON_HOLD:
//                 
//                 break;
//         }
//     }
// }


#include "MilliTimer.h"
#include "arduino.h"

//Uncomment following line to enable debugging serial comments.
//#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// Steps in the greycode checking state machine
enum stepStateEnum
{
    IDLE, 
    ACW1, 
    ACW2, 
    ACW3, 
    CW1, 
    CW2, 
    CW3
};

// Actions to register and report back to main program
enum resultEnum
{
    NO_CHANGE,
    CW_RATE1, 
    CW_RATE2, 
    CW_RATE3, 
    ACW_RATE1, 
    ACW_RATE2, 
    ACW_RATE3, 
    BUTTON_UP, 
    BUTTON_DOWN, 
    BUTTON_DOUBLECLICK, 
    BUTTON_HOLD
};

class Encoder
{
    public:
        Encoder(uint8_t pinA, uint8_t pinB);
        Encoder(uint8_t pinA, uint8_t pinB, uint8_t pinC);
        
        void init(); // Setup pins
        resultEnum poll(bool rateless = false); // checks the state of the encoder and returns new steps if necessary. Call in pinchange interrupt routine or poll in main loop. If polling, do not including blocking code such as delay()!
        resultEnum poll_rateless();

        unsigned int getButtonHoldTime();
        unsigned int getDoubleClickMax();
        unsigned int getDebounceTime();
        unsigned int getRate2Max();
        unsigned int getRate3Max();
        uint8_t getState();
        
        void setButtonHoldTime(unsigned int t);
        void setDoubleClickMax(unsigned int t);
        void setDebounceTime(unsigned int t);
        void setRate2Max(unsigned int t);
        void setRate3Max(unsigned int t);
        void setState(uint8_t state);

        void enableButton();
        void disableButton();
        void enableEncoder();
        void disableEncoder();
        void enable();
        void disable();
    private:
        //pins
        uint8_t _pinA;
        uint8_t _pinB;
        uint8_t _pinC;

        // Encoder enable state
        bool buttonEnabled = true;
        bool encoderEnabled = true;
        
        // button and encoder pin states
        bool A = true;
        bool B = true;
        bool C = true;
        bool Aold = true;
        bool Bold = true;
        bool Cold = true;

        // button settings
        unsigned int buttonDebounceTime = 10;
        unsigned int doubleClickMax = 750;
        unsigned int buttonHoldMin = 1000;

        // encoder settings
        unsigned int rate2Threshold = 125;
        unsigned int rate3Threshold = 25;

        // timers
        MilliTimer buttonDebounceTimer;
        MilliTimer buttonHoldTimer;
        MilliTimer buttonDoubleClickTimer;
        MilliTimer encoderTimer;
        
        // State machine state
        stepStateEnum state = IDLE;
};

#endif