// Ali Maher _ 9932113
// Project Part 1 _ Security
// main code of "master" atmega32

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>

// ==================== Definitions ====================

// === definitions for the keypad === 
#define KEYPAD_PORT PORTA
#define KEYPAD_DDR   DDRA
#define KEYPAD_PIN   PINA
#define ToggleSign  'T'
char ignore;
int key = 0x00;
int encryptionKey = 0x99;
int forbidden = 113;
// ===================================================


// ==================== Functions ====================
int keypadRead();
void transmitDataConfig();
// ===================================================


int main(void) {
    // Communication with slave atmega32
    transmitDataConfig();

    // initial value for keypad
    char buffer[3];

    // === toggle ===
    // led check
    DDRC |= (1 << 0);
    PORTC |= (0 << 0);
    
    GICR = (1 << INT0);                             // set INT0
    MCUCR = 0x03;                                   // MCUCR = 00000011 => for rising edge
    
    sei();                                          // global int. enable

    while (1) {
        _delay_ms(5);
        key = keypadRead();
        if (key == forbidden) continue;

        // === some other cases ===
        if (key == 10) {
            // we reach '*', submitting
            buffer[0] = '*';
        }
        else if (key == 11) {
            key = 0;
            itoa(key, buffer, 10);                  // convert to string for p=showing on lCD
        }
        else if (key == 12) {
            // we reach '#', deleting
            buffer[0] = '#';
        }
        else {
            itoa(key, buffer, 10);                  // convert to string for p=showing on lCD
        }
        
        // === transmitData ===
        PORTB &= ~(1 << PORTB4);                    // select slave
        // == Encrypting data == 
        /* Description: for encrypting data I choose to "xor" the data
        before transmitting with a number as key of encryption. */
        buffer[0] ^= encryptionKey;
        SPDR = buffer[0];
        while(((SPSR >> SPIF) & 1) == 0);           // check if the data sending complete or not
        ignore = SPDR;

        _delay_ms(50);
    }
}


// ========== ISR for toggling int. ========== 
ISR(INT0_vect) {
    PORTC ^= (1 << 0);  
    PORTB &= ~(1 << PORTB4);                        // select slave
    // == Encryption
    char temp = ToggleSign;
    temp ^= encryptionKey;
    // ==
    SPDR = temp;
    while(((SPSR >> SPIF) & 1) == 0);               // check if the data sending complete or not
}
// ===================================================

// ==================== keyPadRead ====================
/* Description:
bcz it doesn't have any voltage (energy) itself, so
we should send VCC from the rows (A, B, C, D), in this
way if the VCC we sent came back from one of the rows
(1, 2, 3), so we can recognize which of the buttons is
pushed.
*/
int keypadRead() {
    int row, col;
    KEYPAD_PORT |= 0x0F;
    for (col = 0;  col < 3; col++) {
        KEYPAD_DDR &= ~(0x7F);                      // the 7-th bit of port A is not used in keypad
        KEYPAD_DDR |= (0x40 >> col);                // we shift to right each time to check which column
        for (row = 0; row < 4; row++) {
            if (!(KEYPAD_PIN & (0x08 >> row))) {    // we shift to right each time to check which row
                // only send data once
                while (!(KEYPAD_PIN & (0x08 >> row)));
                return (row * 3 + col + 1);
            }   
        }
    }
    return forbidden;    // no key pressed
}
// ===================================================

// ==================== transmit data config ====================
/* Description: 
Serial Peripheral Interface (SPI)
SPI Type: Master
SPI Clock Rate: 8MHz / 128 = 62.5 kHz
SPI Clock Phase: Cycle Half
SPI Clock Polarity: Low
SPI Data Order: MSB First
*/
void transmitDataConfig() {
    DDRB = (1 << DDB7) | (0 << DDB6) | (1 << DDB5) | (1 << DDB4);   // SCK, MISO, MOSI, SS_BAR
    PORTB = (1 << PORT4);                          // select slave
    // config: local int. on, turn SPI on, MSB first, Master on, default sampling mode, clock: f_osc/128 
    SPCR = (1 << SPIE) | (1 << SPE) | (0 << DORD) | (1 << MSTR) | (0 << CPOL) | (0 << CPHA) | (1 << SPR1) | (1 << SPR0);  
    SPSR = (0 << SPI2X);  
}
// ===================================================
