//  NOTES:  lfuse should be 0x62 and hfuse should be 0xDF
//          Run at 1Mhz.  See https://eleccelerator.com/fusecalc/fusecalc.php?chip=attiny85 for details
// Read Fuses: avrdude -Cavrdude.conf -pattiny85 -cstk500v1 -PCOM4 -b19200 -U lfuse:r:-:i
// Write Fuses: avrdude -Cavrdude.conf -pattiny85 -cstk500v1 -PCOM4 -b19200 -U lfuse:w:0x62:m -U hfuse:w:0xdf:m
// Board Manager Package: https://raw.githubusercontent.com/sleemanj/optiboot/master/dists/package_gogo_diy_attiny_index.json

#include <avr/sleep.h>
#include "SoftSerial.h"

#define MP3_AMP                       0       // Pull LOW to enable the tiny AMP on the MP3 Player (busy signal is too slow)
#define MP3PLAYER_BUSY                1       // Feedback from the module, LOW while busy
#define SWITCH_PIN                    2       // Trigger/Movement sensor also EXT INT0
#define MP3_TX                        3       // UART TX to MP3 Player
#define MP3PLAYER_POWER               4       // Powers up the Mp3 playing module via a 2N7000 Mosfet

volatile bool soundTriggered = false;

void uint16ToArray(uint16_t value, uint8_t *array) {
  *array = (uint8_t)(value>>8);
  *(array+1) = (uint8_t)(value);
}

uint16_t calculateCheckSum(uint8_t *buffer){
  uint16_t sum = 0;
  for (int i=1; i<7; i++) sum += buffer[i];
  return -sum;
}    

void sendMP3Command(uint8_t command, uint16_t params) {
    uint8_t packet[10];
    packet[0] = 0x7E;
    packet[1] = 0xFF;
    packet[2] = 0x06;
    packet[3] = command;  
    packet[4] = 0;  // No feedback
    packet[9] = 0xEF; // Fin
    uint16ToArray(params,  packet+5);  
    uint16ToArray(calculateCheckSum(packet), packet+7);

    cli();	// turn off interrupts for a clean txmit
    softSerialWrite(packet, 10);
    sei();
}

void playSFX(uint8_t number) {
    digitalWrite(MP3_AMP, HIGH); // Turn the AMP On
    delay(100);
    sendMP3Command(0x03, number);   // 0x03 is "PLAY"
    delay(1000);
    // Wait for it to finish playing
    while (digitalRead(MP3PLAYER_BUSY) == LOW) sleep_mode();
    digitalWrite(MP3_AMP, LOW); // Turn the AMP Off      
}

void sleep() {
    digitalWrite(MP3PLAYER_POWER, LOW);
    pinMode(MP3_TX, INPUT);                 // saves 2uA
    cli();                                  // Disable interrupts    
    GIFR   |= bit (INTF0);                  // Clear any interrupts already registered
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);    
    sei();                                   // Enable interrupts
    sleep_mode();                            // ...and sleep - this is a *very low* power state
    pinMode(MP3_TX, OUTPUT);
    digitalWrite(MP3_TX, HIGH);
    digitalWrite(MP3PLAYER_POWER, HIGH);
    set_sleep_mode(SLEEP_MODE_IDLE);
    delay(650);                              // MP3 Player Powerup Delay
}

void triggerSound() {
    static uint8_t counter = 0;
    // The numbers are THE ORDER IN WHICH THEY ARE COPIED TO THE SD CARD!!!
    switch (counter++) {
        case 0: playSFX(5); break;   // Startup sound
        case 8: playSFX(4); break;   // Ship it  
        case 16: playSFX(1); break;   // Way Too Rude
        case 28: playSFX(3); break;   // Secret
        case 36: playSFX(2); counter = 1; break;  // Seagull and reset
        default: playSFX(2); break;   // Seagull
    }
    // Reset trigger
    soundTriggered = false;
}

void setup() {  
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(MP3PLAYER_POWER, OUTPUT);
    pinMode(MP3_AMP, OUTPUT);    
    pinMode(MP3_TX, OUTPUT);      
    pinMode(MP3PLAYER_BUSY, INPUT); 
    digitalWrite(MP3_TX, HIGH); 
    digitalWrite(MP3_AMP, LOW); 
    digitalWrite(MP3PLAYER_POWER, HIGH);   

    // Turn ADC and Analogue Comparitor OFF, saves ~4mA
    ADCSRA &= ~_BV(ADEN);                   
    ACSR  |= _BV(ACD);  
    // Delay to allow the MP3 player to initialise
    delay(650); 

    // Setup JUST the interrupts we need
    cli();   
    PCMSK = 0;                              // Dont care about pin change interrupts        
    GIFR   |= bit (INTF0);                  // Clear any interrupts already registered
    GIMSK  = _BV(INT0);                     // Enable ONLY INT0 Interrupt (pin 2)
    set_sleep_mode(SLEEP_MODE_IDLE);
    sei();
}

// External Interrupt Request 0 - This will get triggered when pin 2 goes low
ISR(INT0_vect) {
    soundTriggered = true;
}

void loop() {        
    triggerSound();    
    uint32_t start = millis();
    
    // Stay awake for about 3 minutes for "instant" re-trigger
    while (millis() - start < 3 * 60 * 1000UL) {
        if (soundTriggered) {
            triggerSound();
            start = millis();
        }
        sleep_mode();
    }

    sleep();    // goto sleep
}
