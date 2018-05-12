#include "Arduino.h"
#include "EEPROM.h"
#include "WFRChannels.h"

WFRChannels::WFRChannels(byte clockPin, byte latchPin, byte dataPin, unsigned int start_address) {
    init(clockPin, latchPin, dataPin, start_address);
}

WFRChannels::WFRChannels() {
}

void WFRChannels::init(byte clockPin, byte latchPin, byte dataPin, unsigned int start_address) {
    _clockPin = clockPin;
    _latchPin = latchPin;
    _dataPin = dataPin;
    _start_address = start_address;
    
    pinMode(_dataPin, OUTPUT);
    pinMode(_clockPin, OUTPUT);
    pinMode(_latchPin, OUTPUT);
    
    EEPROM.get(_start_address, _value);
    
    hc595write();
    
}

void WFRChannels::hc595write() {
    digitalWrite(_latchPin, LOW);
    for (int i=1; i>=0; i--) {
        shiftOut(_dataPin, _clockPin, MSBFIRST, _value>>(i*8));
    }
    digitalWrite(_latchPin, HIGH);
}

byte WFRChannels::read(byte bit) {
    EEPROM.get(_start_address, _value);
    if (_value & (1 << bit)) {return 1;}
    else {return 0;}
}

unsigned int WFRChannels::read_all(void) {
    EEPROM.get(_start_address, _value);
    return _value;
}

void WFRChannels::write(byte bit, byte bit_value) {
    if (bit_value == 1) {_value ^= 1 << bit;} // устанавливаем нужный бит в единицу
    else {_value &= ~(1 << bit);} // устанавливаем нужный бит в ноль
    
    EEPROM.put(_start_address, _value);
    EEPROM.commit();
    
    hc595write();
}
