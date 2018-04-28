#include "SEHandler.h"

// 0 button and encoder disabled
// 1 button enabled, encoder disabled
// 2 button diasbled, encoder enabled
// 3 both enabled

SEHandler::SEHandler()
{
    // Defaults to Serial1 
    this->HSerial = &Serial1;
} 

SEHandler::SEHandler(HardwareSerial& HSerial)
{
    //HSerial = HSerial;
    this->HSerial = &HSerial;
}

uint8_t SEHandler::init()
{
    // default address is 0.
    return init(1);
}

uint8_t SEHandler::init(uint8_t startAddress)
{
    messageTimeout.init(500);
    HSerial->begin(baudrate);
    // flush the serial buffer.
    while(HSerial->available())
    {
        HSerial->read();
    }
    return setStartAddress(startAddress); // this returns the number of connected units. 0 if it doesn't receive a message.
}

uint8_t SEHandler::setStartAddress(uint8_t startAddress)
{
    messageOut[0] = uint8_tToHex((startAddress) / 16);
    messageOut[1] = uint8_tToHex((startAddress) % 16);
    messageOut[2] = calcChecksum(messageOut, 2);
    
    uint8_t attempt = 0;
    messageTimeout.reset();
    // now need to wait a short while and check for a response. How long do I need to wait?
    while(attempt < maxRetries)
    {
        // Serial.print("Attempt: ");
        // Serial.println(attempt);
        sendMessageOut(3);
        while(!messageTimeout.timedOut(true))
        {
            if(checkSerial())
            {
                if( msgIndex == 3)
                {
                    return messageInAddressToInt() - startAddress;
                }
            }
        }
        attempt++;
    }
    return 0;
}

event_t SEHandler::poll(bool rateless)
{
    event_t event;
    if(checkSerial())
    {
        if(msgIndex == 4) // uint8_t len = msgIndex - 1;
        {
            event.address = messageInAddressToInt();
            switch(messageIn[2])
            {
                case '0':
                    event.result = CW_RATE1;
                    break;
                case '1':
                    event.result = CW_RATE2;
                    break;
                case '2':
                    event.result = CW_RATE3;
                    break;
                case '3':
                    event.result = ACW_RATE1;
                    break;
                case '4':
                    event.result = ACW_RATE2;
                    break;
                case '5':
                    event.result = ACW_RATE3;
                    break;
                case '6':
                    event.result = BUTTON_UP;
                    break;
                case '7':
                    event.result = BUTTON_DOWN;
                    break;
                case '8':
                    event.result = BUTTON_DOUBLECLICK;
                    break;
                case '9':
                    event.result = BUTTON_HOLD;
                    break;
                default:
                    event.result = NO_CHANGE;
                    break;
            }
        }
    }
    if(rateless) // just return rate1 so less checks have to be made if they aren't wanted.
    {
        if     (event.result == CW_RATE2 || event.result == CW_RATE3)   { event.result = CW_RATE1; }
        else if(event.result == ACW_RATE2 || event.result == ACW_RATE3) { event.result = ACW_RATE1; }
    }
    return event;
}

event_t SEHandler::poll_rateless()
{
    return poll(true);
}

bool SEHandler::setButtonHoldTime(uint8_t address, uint16_t t)
{
    return sendValue(address, 'H', t);
}

bool SEHandler::setDoubleClickMax(uint8_t address, uint16_t t)
{
    return sendValue(address, 'C', t);
}

bool SEHandler::setDebounceTime(uint8_t address, uint16_t t)
{
    return sendValue(address, 'D', t);
}

bool SEHandler::setRate2Max(uint8_t address, uint16_t t)
{
    return sendValue(address, '2', t);
}

bool SEHandler::setRate3Max(uint8_t address, uint16_t t)
{
    return sendValue(address, '3', t);
}

bool SEHandler::setState(uint8_t address, uint8_t state)
{
    // set the state of the encoder
    // 0 button and encoder disabled
    // 1 button enabled, encoder disabled
    // 2 button disabled, encoder enabled
    // 3 both enabled
    return sendValue(address, 'S', state);
}

uint16_t SEHandler::getButtonHoldTime(uint8_t address)
{
    return getValue(address, 'H');
}

uint16_t SEHandler::getDoubleClickMax(uint8_t address)
{
    return getValue(address, 'H');
}

uint16_t SEHandler::getDebounceTime(uint8_t address)
{
    return getValue(address, 'D');
}

uint16_t SEHandler::getRate2Max(uint8_t address)
{
    return getValue(address, '2');
}

uint16_t SEHandler::getRate3Max(uint8_t address)
{
    return getValue(address, '3');
}

uint8_t SEHandler::getState(uint8_t address)
{
    return getValue(address, 'S');
}

void SEHandler::sendMessageOut(uint8_t len)
{
    HSerial->write(STX);
    for(int i = 0; i < len; i++)
    {
        HSerial->write(messageOut[i]);
        // Serial.write(messageOut[i]);
    }
    HSerial->write(LF);
    // Serial.write(LF);
}

char SEHandler::uint8_tToHex(uint8_t n)
{
    if(n < 10) { return n + 48; }
    else       { return n + 55; }
}

char SEHandler::calcChecksum(char* rawMessage, int len)
{
    uint16_t checksum=0;
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

bool SEHandler::checkSerial()
{
    while(HSerial->available())
    {
        char in = HSerial->read();
        
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
                    // Serial.write(LF);
                    serialState = WAITING_FOR_START;
                    // Calculate checksum and make sure it matches. If it does, parse the message
                    uint8_t msgLen = msgIndex - 1;
                    if(messageIn[msgLen] == calcChecksum(messageIn, msgLen))
                    {
                        // Parse message
                        //HSerial->write(STX);
                        //HSerial->print("Received good message.");
                        //HSerial->write(LF);
                        return true; // message has been received
                    }
                }
                else if(in == STX)
                {
                    //something must have glitched so reset msgIndex to start again
                    msgIndex = 0;
                }
                else if(msgIndex < LEN)
                {
                    // Serial.write(in);
                    messageIn[msgIndex] = in;
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
    return false; //no new message
}

uint8_t SEHandler::messageInAddressToInt()
{
    return 16 * hexTo_uint8_t(messageIn[0]) + hexTo_uint8_t(messageIn[1]);
}

uint8_t SEHandler::nDigits(uint16_t i)
{
    if(i > 9999)    { return 5;}
    else if(i > 999){ return 4;}
    else if(i > 99) { return 3;}
    else if(i > 9)  { return 2;}
    else            { return 1;}
}

bool SEHandler::sendValue(uint8_t address, char var, uint16_t value)
{
    // Send an update value to a smart encoder unit. Checks for a response to show that the unit was successfully set. Returns a success or failure bool after the max number of attempts.
    messageOut[0] = uint8_tToHex((address) / 16);
    messageOut[1] = uint8_tToHex((address) % 16);
    messageOut[2] = var;

    uint8_t msgLen = 3 + nDigits(value);
    for(int i = 0; i < msgLen - 3; i++)
    {
        messageOut[msgLen - 1 - i] = (value % 10) + '0';
        value /= 10;
    }

    messageOut[msgLen] = calcChecksum(messageOut, msgLen);

    uint8_t attempt = 0;
    messageTimeout.reset();
    while(attempt < maxRetries)
    {
        // Serial.print("Attempt: ");
        // Serial.println(attempt);
        sendMessageOut(msgLen + 1);
        while(!messageTimeout.timedOut(true))
        {
            if(checkSerial())
            {
                bool match = true;
                for(int i = 0; i < msgLen + 1; i++)
                {
                    if(messageOut[i] != messageIn[i])
                    {
                        match = false;
                    }
                }
                if(match)
                {
                    return true;
                }
            }
        }
        attempt++;
    }
    return false;
}

uint16_t SEHandler::getValue(uint8_t address, char var)
{
    messageOut[0] = uint8_tToHex((address) / 16);
    messageOut[1] = uint8_tToHex((address) % 16);
    messageOut[2] = var;
    messageOut[3] = calcChecksum(messageOut, 3);

    uint8_t attempt = 0;
    messageTimeout.reset();
    while(attempt < maxRetries)
    {
        // Serial.print("Attempt: ");
        // Serial.println(attempt);
        sendMessageOut(4);
        while(!messageTimeout.timedOut(true))
        {
            if(checkSerial())
            {
                // see if this is a response from the correct channel and if so, get the value
                bool match = true;
                for(int i = 0; i < 3; i++)
                {
                    if(messageOut[i] != messageIn[i])
                    {
                        match = false;
                    }
                }
                if(match)
                {
                    return messageToInt(3);
                }
            }
        }
        attempt++;
    }
    return 0;
}

uint16_t SEHandler::messageToInt(uint8_t startIndex)
{
    // Returns the number stored in a char array, starting at startIndex
    uint16_t number = 0;
    // msgIndex - 1 because we don't include the checksum!
    for(int i = startIndex; i < msgIndex - 1; i++) 
    {
        number *= 10;
        number += (messageIn[i] -'0');
    }
    return number;
}

uint8_t SEHandler::hexTo_uint8_t(char c)
{
    // If c - 48 is less than 10, return it. Otherwise, subtract a bit more because c must have been A to F.
    c -= '0';
    if(c < 10) { return c; }
    else       { return c - 7;}
}