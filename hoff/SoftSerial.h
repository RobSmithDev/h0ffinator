#ifndef SoftwareSerial_h
#define SoftwareSerial_h

#include <avr/io.h>

#define SERPORT	PORTB
#define TXPIN	PB3

typedef uint8_t byte;

void softSerialBegin();
void softSerialWrite(uint8_t* b, uint8_t numBytes);

#endif