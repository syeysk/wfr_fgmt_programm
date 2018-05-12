/*
 * 
 * WFR is WIFi Relay
 * 
 * http://mypractic.ru/urok-9-sozdanie-biblioteki-dlya-arduino.html
 * 
 * You must initiate this class after you initiate EEPROM in your sketch.
 * 
 * */

// проверка, что библиотека еще не подключена
#ifndef WFRChannels_h  // если библиотека Button не подключена
#define WFRChannels_h  // тогда подключаем ее

#include "Arduino.h"

class WFRChannels {
    public:
        WFRChannels(byte clockPin, byte latchPin, byte dataPin, unsigned int start_address);
        WFRChannels();
        void init(byte clockPin, byte latchPin, byte dataPin, unsigned int start_address);
        //void read(byte num);
        void hc595write();
        byte read(byte bit);
        void write(byte bit, byte bit_value);
        unsigned int read_all(void);

    private:
        byte _clockPin;
        byte _latchPin;
        byte _dataPin;
        unsigned int _value;
        unsigned int _start_address;
};

#endif
