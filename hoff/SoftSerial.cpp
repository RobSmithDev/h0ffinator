#include "SoftSerial.h"

#if F_CPU == 1000000
#define TX_DELAY    11
const int XMIT_START_ADJUSTMENT = 3;
#elif F_CPU == 8000000
// Was 112, but with LTO is too fast! 
#define TX_DELAY 118
const int XMIT_START_ADJUSTMENT = 4;
#endif


// from https://github.com/sirleech/NewSoftSerial/blob/master/NewSoftSerial.cpp
inline void tunedDelay(uint16_t delay) {
    uint8_t tmp = 0;

    asm volatile("sbiw    %0, 0x01 \n\t"
        "ldi %1, 0xFF \n\t"
        "cpi %A0, 0xFF \n\t"
        "cpc %B0, %1 \n\t"
        "brne .-10 \n\t"
        : "+w" (delay), "+a" (tmp)
        : "0" (delay)
    );
}

void softSerialBegin() {
  tunedDelay(TX_DELAY);
}

void softSerialWrite(uint8_t* b, uint8_t numBytes) {	
    tunedDelay(TX_DELAY + XMIT_START_ADJUSTMENT);

    while (numBytes--) {
        SERPORT &= ~(1<<TXPIN); // tx pin low
        tunedDelay(TX_DELAY);
        // Write each of the 8 bits
        for (byte mask = 0x01; mask; mask <<= 1) {
            if (*b & mask) // choose bit
                SERPORT |= (1<<TXPIN); // tx pin high, send 1
            else
                SERPORT &= ~(1<<TXPIN); // tx pin low, send 0

            tunedDelay(TX_DELAY);
        }
        b++;
        SERPORT |= (1<<TXPIN); // tx pin high, restore pin to natural state
        tunedDelay(TX_DELAY);
    }
}

