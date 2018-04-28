#include "Encoder.h"
#include "arduino.h"
#include "MilliTimer.h"


Encoder::Encoder(uint8_t pinA, uint8_t pinB)
{
    _pinA = pinA;
    _pinB = pinB;
    buttonEnabled = false;
}

Encoder::Encoder(uint8_t pinA, uint8_t pinB, uint8_t pinC)
{
    _pinA = pinA;
    _pinB = pinB;
    _pinC = pinC;
}

void Encoder::init()
{
    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    if(buttonEnabled) { pinMode(_pinC, INPUT_PULLUP); }

    buttonDebounceTimer.updateTimeOut(buttonDebounceTime);
    buttonHoldTimer.updateTimeOut(buttonHoldMin);
    buttonDoubleClickTimer.updateTimeOut(doubleClickMax);
}

resultEnum Encoder::poll(bool rateless)
{
    
    if(buttonEnabled)
    {
        Cold = C;
        C = digitalRead(_pinC);
        if(C != Cold && buttonDebounceTimer.timedOut(true))
        {
            // Button state has changed
            if(C) //Button has been pressed
            {
                if(!buttonDoubleClickTimer.timedOut(true))
                {
                    return BUTTON_DOUBLECLICK;
                }
                else
                {
                    buttonDoubleClickTimer.reset();
                    return BUTTON_UP;
                }
            }
            else // Button has been pressed or released
            {
                buttonHoldTimer.reset();
                return BUTTON_DOWN;
            }
        }
        else if(!C) // button is being held down
        {
            if(buttonHoldTimer.timedOut(true))
            {
                return BUTTON_HOLD;
            }
        }
    }

    if(encoderEnabled)
    {
        Aold = A;
        Bold = B;
        A = digitalRead(_pinA);
        B = digitalRead(_pinB);
        if(A != Aold || B != Bold)
        {
            // Something has changed so evaluate
            DEBUG_PRINT(A);
            DEBUG_PRINT(B);
            DEBUG_PRINT(": ");
            switch(state)
            {
                case IDLE:
                    //DEBUG_PRINTLN("IDLE");
                    if(!A && B)
                    {
                        // Start of anticlockwise turn
                        DEBUG_PRINTLN("IDLE to ACW1");
                        state = ACW1;
                    }
                    else if(A && !B)
                    {
                        // Start of clockwise turn
                        DEBUG_PRINTLN("IDLE to CW1");
                        state = CW1;
                    }
                    else
                    {
                        // Reading not valid so ignore
                        DEBUG_PRINTLN("Invalid so IDLE to IDLE");
                        state = IDLE;
                    }
                    break;
                case ACW1:
                    //DEBUG_PRINTLN("ACW1");
                    if(!A && !B)
                    {
                        // Valid move so increment state
                        DEBUG_PRINTLN("ACW1 to ACW2");
                        state = ACW2;
                    }
                    else if(A && B)
                    {
                        // Bounced back to idle
                        DEBUG_PRINTLN("ACW1 to IDLE");
                        state = IDLE;
                    }
                    else
                    {
                        // Illegal reading so will have to reset
                        DEBUG_PRINTLN("Invalid so ACW1 to IDLE");
                        state = IDLE;
                    }
                    break;
                case ACW2:
                    //DEBUG_PRINTLN("ACW2");
                    if(A && !B)
                    {
                        // Valid move so increment state
                        DEBUG_PRINTLN("ACW2 to ACW3");
                        state = ACW3;
                    }
                    else if(!A && B)
                    {
                        // bounced backwards
                        DEBUG_PRINTLN("ACW2 to ACW1");
                        state = ACW1;
                    }
                    else
                    {
                        // Illegal reading so will have to reset
                        DEBUG_PRINTLN("Invalid so ACW2 to IDLE");
                        state = IDLE;
                    }
                    break;
                case ACW3:
                    //DEBUG_PRINTLN("ACW3");
                    if(A && B)
                    {
                        // One whole step completed!
                        DEBUG_PRINTLN("Valid Step, Increment to IDLE");
                        state = IDLE;
                        unsigned int encoderTimeDiff = encoderTimer.elapsed();
                        encoderTimer.reset();
                        if(encoderTimeDiff < rate3Threshold && !rateless)
                        {
                            return ACW_RATE3;
                        }
                        else if(encoderTimeDiff < rate2Threshold && !rateless)
                        {
                            return ACW_RATE2;
                        }
                        else
                        {
                            return ACW_RATE1;
                        }
                    }
                    else if(!A && !B)
                    {
                        // Bounced backwards so decrement
                        DEBUG_PRINTLN("ACW3 to ACW2");
                        state = ACW2;
                    }
                    else
                    {
                        // Invalid so reset
                        DEBUG_PRINTLN("Invalid so ACW3 to IDLE");
                        state = IDLE;
                    }
                    break;
                case CW1:
                    //DEBUG_PRINTLN("CW1");
                    if(!A && !B)
                    {
                        // Valid move so increment state
                        DEBUG_PRINTLN("CW1 to CW2");
                        state = CW2;
                    }
                    else if(A && B)
                    {
                        // Bounced back to idle
                        DEBUG_PRINTLN("CW1 to IDLE");
                        state = IDLE;
                    }
                    else
                    {
                        // Illegal reading so will have to reset
                        DEBUG_PRINTLN("Invalid so CW1 to IDLE");
                        state = IDLE;
                    }
                    break;
                case CW2:
                    //DEBUG_PRINTLN("CW2");
                    if(!A && B)
                    {
                        // Valid move so increment state
                        DEBUG_PRINTLN("CW2 to CW3");
                        state = CW3;
                    }
                    else if(A && !B)
                    {
                        // bounced backwards
                        DEBUG_PRINTLN("CW2 to CW1");
                        state = CW1;
                    }
                    else
                    {
                        // Illegal reading so will have to reset
                        DEBUG_PRINTLN("Invalid so CW2 to IDLE");
                        state = IDLE;
                    }
                    break;
                case CW3:
                    //DEBUG_PRINTLN("CW3");
                    if(A && B)
                    {
                        // One whole step completed!
                        DEBUG_PRINTLN("Valid Step, Increment to IDLE");
                        state = IDLE;
                        unsigned int encoderTimeDiff = encoderTimer.elapsed();
                        encoderTimer.reset();
                        if(encoderTimeDiff < rate3Threshold && !rateless)
                        {
                            return CW_RATE3;
                        }
                        else if(encoderTimeDiff < rate2Threshold && !rateless)
                        {
                            return CW_RATE2;
                        }
                        else
                        {
                            return CW_RATE1;
                        }
                    }
                    else if(!A && !B)
                    {
                        // Bounced backwards so decrement
                        DEBUG_PRINTLN("CW3 to CW2");
                        state = CW2;
                    }
                    else
                    {
                        // Invalid so reset
                        DEBUG_PRINTLN("Invalid so CW3 to IDLE");
                        state = IDLE;
                    }
                    break;
            }
        }
    }
    return NO_CHANGE;
}

resultEnum Encoder::poll_rateless()
{
    return poll(true);
}

unsigned int Encoder::getButtonHoldTime()
{
    return buttonHoldMin;
}

unsigned int Encoder::getDoubleClickMax()
{
    return doubleClickMax;
}

unsigned int Encoder::getDebounceTime()
{
    return buttonDebounceTime;
}

unsigned int Encoder::getRate2Max()
{
    return rate2Threshold;
}

unsigned int Encoder::getRate3Max()
{
    return rate3Threshold;
}

void Encoder::setButtonHoldTime(unsigned int t)
{
    buttonHoldMin = t;
    buttonHoldTimer.updateTimeOut(buttonHoldMin);
}

void Encoder::setDoubleClickMax(unsigned int t)
{
    doubleClickMax = t;
    buttonDoubleClickTimer.updateTimeOut(doubleClickMax);
}

void Encoder::setDebounceTime(unsigned int t)
{
    buttonDebounceTime = t;
    buttonDebounceTimer.updateTimeOut(buttonDebounceTime);
}

void Encoder::setRate2Max(unsigned int t)
{
    rate2Threshold = t;
}

void Encoder::setRate3Max(unsigned int t)
{
    rate3Threshold = t;
}

void Encoder::setState(uint8_t state)
{
    switch(state)
    {
        case 0:
            encoderEnabled = false;
            buttonEnabled = false;
            break;
        case 1:
            encoderEnabled = false;
            buttonEnabled = true;
            break;
        case 2:
            encoderEnabled = true;
            buttonEnabled = false;
            break;
        case 3:
            encoderEnabled = true;
            buttonEnabled = true;
            break;
        default:
            break;
    }
}

uint8_t Encoder::getState()
{
    if(buttonEnabled && encoderEnabled){ return 3; }
    else if(encoderEnabled)            { return 2; }
    else if(buttonEnabled)             { return 1; }
    else                               { return 0; }
}

void Encoder::enableButton()
{
    buttonEnabled = true;
}

void Encoder::disableButton()
{
    buttonEnabled = false;
}

void Encoder::enableEncoder()
{
    encoderEnabled = true;
}

void Encoder::disableEncoder()
{
    encoderEnabled = false;
}

void Encoder::enable()
{
    buttonEnabled = true;
    encoderEnabled = true;
}

void Encoder::disable()
{
    buttonEnabled = false;
    encoderEnabled = false;
}
