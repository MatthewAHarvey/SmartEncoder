#ifndef SEHANDLER_H
#define SEHANDLER_H

#include<arduino.h>
// #include<HardwareSerial.h>
#include "Encoder.h"
#include "MilliTimer.h"
//#include<SoftwareSerial.h>
//#include "SerialHelper.h"

// To do
// class with serial port

// extern HardwareSerial Serial1(2);

enum serialModes{WAITING_FOR_START, COLLECTING_MESSAGE};

// enum resultEnum // Actions to register and report back to main program
// {
//     NO_CHANGE,
//     CW_RATE1, 
//     CW_RATE2, 
//     CW_RATE3, 
//     ACW_RATE1, 
//     ACW_RATE2, 
//     ACW_RATE3, 
//     BUTTON_UP, 
//     BUTTON_DOWN, 
//     BUTTON_DOUBLECLICK, 
//     BUTTON_HOLD
// };

struct event_t
{
    uint8_t address = 0;
    resultEnum result = NO_CHANGE;
};

struct value_t
{
    uint8_t address = 0;
    char var = '\0';
    uint16_t value = 0;
};

class SEHandler
{
    public:
        //SEHandler(); // Defaults to Serial1, reverseDirection swaps all the encoder rotation directions globally.
        SEHandler(HardwareSerial& HSerial);
        // SEHandler(bool reverseDirection); // Defaults to Serial1, reverseDirection swaps all the encoder rotation directions globally.
        // SEHandler(HardwareSerial& HSerial, bool reverseDirection);

        uint8_t init(); // Defaults to address 0.
        uint8_t init(uint8_t startAddress);
        uint8_t setStartAddress(uint8_t startAddress);
        event_t poll(bool rateless = false);
        event_t poll_rateless();

        bool setButtonHoldTime(uint8_t address, uint16_t t);
        bool setDoubleClickMax(uint8_t address, uint16_t t);
        bool setDebounceTime(uint8_t address, uint16_t t);
        bool setRate2Max(uint8_t address, uint16_t t);
        bool setRate3Max(uint8_t address, uint16_t t);
        bool setState(uint8_t address, uint8_t state);
        bool setUseReverseDirection(uint8_t address, bool reverseDirection);

        uint16_t getButtonHoldTime(uint8_t address);
        uint16_t getDoubleClickMax(uint8_t address);
        uint16_t getDebounceTime(uint8_t address);
        uint16_t getRate2Max(uint8_t address);
        uint16_t getRate3Max(uint8_t address);
        uint8_t getState(uint8_t address);
        bool getUseReverseDirection(uint8_t address);

    private: 
        // __________Variables___________   
        serialModes serialState;
        const char STX = 0x02; // start transmission ascii code
        const char LF = 0x0A; // line feed ascii code
        uint8_t msgIndex;
        static const uint8_t LEN = 8;
        char messageIn[LEN]; // max length of a message. NOT terminated by \0   
        char messageOut[LEN];
        
        const long baudrate = 125000;

        HardwareSerial* HSerial;
        
        uint8_t nEncoders; // number of smart encoders
        uint8_t startAddress;
        char address[2] = {'0', '0'};
        uint8_t addressInt = 0;

        MilliTimer messageTimeout;
        uint8_t timeOut = 10; //ms to wait before giving up on receiving commands
        uint8_t maxRetries = 2;
        // __________Functions__________
        void sendMessageOut(uint8_t len);
        char uint8_tToHex(uint8_t n);
        char calcChecksum(char* rawMessage, int len);
        bool checkSerial(); // returns true if there is a new message that passes checksum
        uint8_t messageInAddressToInt(); // convert message hex chars to uint
        uint8_t nDigits(uint16_t i); // returns the number of digits in an unsigned integer number
        bool sendValue(uint8_t address, char var, uint16_t value);
        uint16_t getValue(uint8_t address, char var);
        uint16_t messageToInt(uint8_t startIndex);
        uint8_t hexTo_uint8_t(char c);
};

#endif