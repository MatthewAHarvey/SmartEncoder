// ATTINY85 code for rotary encoder
// 16 MHz PLL works well.
// Must flash FreqCal and adjust oscillator with oscilloscope or frequency counter to ensure serial works properly
// Tested with 3 encoders and a hardware serial port on a mega, each message takes 5us and I can generate over 400 events per second when turning an encoder as fast as I can. It goes even faster when I turn two encoders. There will be misses but this is still a responsive system.

#include "Encoder.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>

const long baudrate = 125000;
const int tx = 0; // software serial transmit. pin 5
const int rx = 4; // software serial receive. pin 3
SoftwareSerial SSerial(rx, tx);

// define pins
const int encPinA = 2; //pin 7, PCINT2
const int encPinB = 1; //pin 6, PCINT1
const int encPinC = 3; //pin 2, PCINT3
Encoder enc(encPinA, encPinB, encPinC);

// start and stop chars
const char STX = 0x02; // start transmission ascii code
const char LF = 0x0A; // line feed ascii code

uint8_t msgIndex;
static const uint8_t LEN = 9;
char message[LEN]; // max length of a message. NOT terminated by \0   

//int address = -1;
char address[] = "00";
uint8_t addressInt = 0;

//typedef enum {WAITING_FOR_ADDRESS, RUNNING} runModes;
//typedef enum {WAITING_FOR_START, COLLECTING_MESSAGE} serialModes;
enum runModes{WAITING_FOR_ADDRESS, RUNNING};
enum serialModes{WAITING_FOR_START, COLLECTING_MESSAGE};

runModes state;
serialModes serialState;

// Protocol:
// No rotary encoder events will be registered until the unit has received an address.
// The address takes the form of two hex chars to allow the address of a unit to be set between 0 and 254. All units respond to 255 since this is the global update address.
// The master must send <STX><Address1><Address0><checksum><LF> to set the first unit's address. This unit then increments this address and passes it on. Do not overflow 254!!!
// Set the debounce with <STX><A1><A0><D><n><checksum><LF> where n is the debounce time in milliseconds.
// Enable or disable the encoder with <STX><A1><A0><E><n><checksum><LF> where n is 1 or 0.

// Except for address messages, all messages are echoed.
// Rotary encoder events are broadcast <STX><A1><A0><x><checksum><LF> where A1, A0 give the encoder address 
// and x is 0 for clockwise step at rate 1
//          1 for clockwise step at rate 2
//          2 for clockwise step at rate 3
//          3 for anti-clockwise step at rate 1
//          4 for anti-clockwise step at rate 2
//          5 for anti-clockwise step at rate 3
//          6 for button up
//          7 for button down
//          8 for button doubleclick
//          9 for button hold


void setup()
{
    updateOscFreq();

    enc.init();
    SSerial.begin(baudrate);
    //state = RUNNING;
    state = WAITING_FOR_ADDRESS;
    serialState = WAITING_FOR_START;    
}

void loop()
{
    checkSerial();
    checkEncoder();
}

void checkEncoder()
{
    if(state == RUNNING)
    {
        switch(enc.poll())
        {
            case NO_CHANGE:
                break;
            case CW_RATE1: 
                sendState(0);
                break;
            case CW_RATE2:
                sendState(1);
                break;
            case CW_RATE3: 
                sendState(2);
                break;
            case ACW_RATE1: 
                sendState(3);
                break;
            case ACW_RATE2: 
                sendState(4);
                break;
            case ACW_RATE3: 
                sendState(5);
                break;
            case BUTTON_UP: 
                sendState(6);
                break;
            case BUTTON_DOWN: 
                sendState(7);
                break;
            case BUTTON_DOUBLECLICK: 
                sendState(8);
                break;
            case BUTTON_HOLD:
                sendState(9);
                break;
        }
    }
}

void updateOscFreq()
{
    if(EEPROM.read(0) == 'C' && EEPROM.read(1) == 'A' && EEPROM.read(2) == 'L')
    {
        OSCCAL = EEPROM.read(3);
    }
}

void sendState(int i)
{
    if(0 <= i && i < 10)
    {
        message[0] = address[0];
        message[1] = address[1];
        message[2] = i + '0';
        message[3] = calcChecksum(message, 3);

        SSerial.write(STX);
        for(int i = 0; i < 4; i++)
        {
            SSerial.write(message[i]);
        }
        SSerial.write(LF);
    }
}

void checkSerial()
{
    while(SSerial.available())
    {
        char in = SSerial.read();
        
        switch(serialState)
        {
            case WAITING_FOR_START:
                if(in == STX)
                {
                    serialState = COLLECTING_MESSAGE;
                    msgIndex = 0;
                }
                break;
            case COLLECTING_MESSAGE:
                if(in == LF)
                {
                    serialState = WAITING_FOR_START;
                    // Calculate checksum and make sure it matches. If it does, parse the message
                    uint8_t msgLen = msgIndex - 1;
                    if(message[msgLen] == calcChecksum(message, msgLen))
                    {
                        // Parse message
                        //SSerial.write(STX);
                        //SSerial.print("Received good message.");
                        //SSerial.write(LF);
                        parseMessage();
                    }
                }
                else if(in == STX)
                {
                    //something must have glitched so reset msgIndex to start again
                    msgIndex = 0;
                }
                else if(msgIndex < LEN)
                {
                    message[msgIndex] = in;
                    msgIndex++;
                }
                else
                {
                    //message would now be too long so go back to waiting for a new reading
                    serialState = WAITING_FOR_START;
                }
                break;
        }
    }
}

void parseMessage()
{
    char type = message[2];
    unsigned int value = 0;
    switch(msgIndex)
    {
        case 3: // set address and set mode to RUNNING
            if(validAddress())
            {
                address[0] = message[0];
                address[1] = message[1];
                state = RUNNING;
                // Now we need to increment the address and pass it on. To do this, convert the address to an int, increment it, convert it back to hex array then send this.
                sendNextSetAddress();
            }
            break;
        case 4: // get value of something
            //char type = message[2];
            if(addressMatch())
            {
                message[0] = address[0]; // update these in case the message was to all units.
                message[1] = address[1];
                switch(type)
                {
                    case '2': // rate 2 max (ms)
                        value = enc.getRate2Max();
                        break;
                    case '3': // rate 3 max (ms)
                        value = enc.getRate3Max();
                        break;
                    case 'C': // double click max (ms)
                        value = enc.getDoubleClickMax();
                        break;
                    case 'D': // button debounce time (ms)
                        value = enc.getDebounceTime();
                        break;
                    case 'H': // hold button min (ms)
                        value = enc.getButtonHoldTime();
                        break;
                    case 'S': // set the state of the encoder
                        // 0 button and encoder disabled
                        // 1 button enabled, encoder disabled
                        // 2 button diasbled, encoder enabled
                        // 3 both enabled
                        value = enc.getState();
                        break;
                }
                sendValue(value);
            }
            else
            {
                echoSerial();
            }
            break;
        case 5: // If the length is between 5 and 8, a value is being set between 0 and 9999 (usually ms).
        case 6:
        case 7:
        case 8:
            // Echo the message so that it reaches the master. This way the master can check setting messages were received, and any encoder updates get passed along.
            echoSerial();
            // Now check whether I need to set something
            if(addressMatch() || globalMatch())
            {
                // The message is for us so parse it
                //char type = message[2];
                int value = messageToInt(3);
                switch(type)
                {
                    case '2': // rate 2 max (ms)
                        enc.setRate2Max(value);
                        break;
                    case '3': // rate 3 max (ms)
                        enc.setRate3Max(value);
                        break;
                    case 'C': // double click max (ms)
                        enc.setDoubleClickMax(value);
                        break;
                    case 'D': // button debounce time (ms)
                        enc.setDebounceTime(value);
                        break;
                    case 'H': // hold button min (ms)
                        enc.setButtonHoldTime(value);
                        break;
                    case 'S': // set the state of the encoder
                        // 0 button and encoder disabled
                        // 1 button enabled, encoder disabled
                        // 2 button diasbled, encoder enabled
                        // 3 both enabled
                        enc.setState(value);
                        break;
                }
            }
            break;
    }
}

void sendValue(unsigned int value)
{
    message[0] = address[0];
    message[1] = address[1];
    uint8_t msgLen = 3 + nDigits(value);
    for(int i = 0; i < msgLen - 3; i++)
    {
        message[msgLen - 1 - i] = (value % 10) + '0';
        value /= 10;
    }

    SSerial.write(STX);
    for(int i = 0; i < msgLen; i++)
    {
        SSerial.write(message[i]);
    }
    SSerial.write(calcChecksum(message, msgLen));
    SSerial.write(LF);
}

void sendNextSetAddress()
{
    // Works out what number address this unit has, increments it and sends that on as two hexidecimal digits to the next unit.
    uint8_t addressInt = 16 * hexTo_uint8_t(address[0]) + hexTo_uint8_t(address[1]);
    message[0] = uint8_tToHex((addressInt + 1) / 16);
    message[1] = uint8_tToHex((addressInt + 1) % 16);
    message[2] = calcChecksum(message, 2);
    
    SSerial.write(STX);
    for(int i = 0; i < 3; i++)
    {
        SSerial.write(message[i]);
    }
    SSerial.write(LF);
}

bool addressMatch()
{
    if(message[0] == address[0] && message[1] == address[1]) { return true; }
    else                                                     { return false; }
}

bool globalMatch()
{
    if(message[0] == 'F' && message[1] == 'F') { return true; }
    else                                       { return false; }
}

bool validAddress()
{
    // both A1 and A0 must be a hex numeral
    for(int i = 0; i < 2; i++)
    {
        if(message[i] < '0' || message[i] > 'F' || ('9' < message[i] && message[i] < 'A')) 
        {
            return false;
        }
        return true;
    }
}

char uint8_tToHex(uint8_t n)
{
    if(n < 10) { return n + 48; }
    else       { return n + 55; }
}

int messageToInt(int startIndex)
{
    // Returns the number stored in a char array, starting at startIndex
    int number = 0;
    for(int i = startIndex; i < msgIndex - 1; i++) // msgIndex - 1 because we don't include the checksum!
    {
        number *= 10;
        number += (message[i] -'0');
    }
    return number;
}

uint8_t hexTo_uint8_t(char c)
{
    // If c - 48 is less than 10, return it. Otherwise, subtract a bit more because c must have been A to F.
    c -= '0';
    if(c < 10) { return c; }
    else       { return c - 7;}
}

uint8_t nDigits(unsigned int i)
{
    if(i > 9999)    { return 5;}
    else if(i > 999){ return 4;}
    else if(i > 99) { return 3;}
    else if(i > 9)  { return 2;}
    else            { return 1;}
}

void echoSerial()
{
    // Send the message buffer on
    SSerial.write(STX);
    //SSerial.print("Echo: ");
    for(int i = 0; i < msgIndex; i++)
    {
        SSerial.write(message[i]);
    }
    SSerial.write(LF);
}

char calcChecksum(char* rawMessage, int len)
{
    unsigned int checksum=0;
    for(int i = 0; i < len; i++)
    { //add the command
        checksum += rawMessage[i];
    }
    //Calculate checksum based on MPS manual
    checksum = ~checksum+1; //the checksum is currently a unsigned 16bit int. Invert all the bits and add 1.
    checksum = 0x7F & checksum; // discard the 8 MSBs and clear the remaining MSB (B0000000001111111)
    checksum = 0x40 | checksum; //bitwise or bit6 with 0x40 (or with a seventh bit which is set to 1.) (B0000000001000000)
    return (char) checksum;
}
