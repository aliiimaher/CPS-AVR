// Ali Maher _ 9932113
// CPS Project of MP
// main code of "master" atmega32

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <LCD.h>
#include <avr/interrupt.h>
#include <math.h>


// ==================== Definitions ====================
// === definitions for the keypad === 
#define KEYPAD_PORT PORTC
#define KEYPAD_DDR   DDRC
#define KEYPAD_PIN   PINC
// === Data Communication ===
#define ToggleSign  'T'
char ignore;
int key = 0x00;
int encryptionKey = 0x99;
int forbidden = 113;
// === ADC ===
float celsius = 27.00;
float prev_celsius = 27.00;
char temperature[4];
// === Motors ===
int counter = 0;                                    // trick for handling data overrun 
// === Lightening ===
float ldrValue;
// ===================================================


// ==================== Functions ====================
int keypadRead();
void transmitDataConfig();
void adcConfig_LM35();
int adc_read_LM35();
void adcConfig_LDR();
int adc_read_LDR();
// ===================================================


int main(void) {
    // ADC initialization for LM35
    adcConfig_LM35(); 

    // Communication with slave atmega32
    transmitDataConfig();

    // initial value for keypad
    char buffer[3];

    // === toggle ===
    // led check
    DDRD |= (1 << 0);
    PORTD |= (0 << 0);
    
    GICR |= (1 << INT0);                            // set INT0
    MCUCR = 0x03;                                   // MCUCR = 00000011 => for rising edge
    
    sei();                                          // global int. enable

    while (1) {
        _delay_ms(200);
        goto LDR;
        _delay_ms(250);
        // ========================= Part 1 ========================= 
        _delay_ms(5);
        key = keypadRead();
        // _delay_ms(300);
        if (key == forbidden) goto ADC_Part;

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

        _delay_ms(5);
        // ========================= =========================


    ADC_Part:
        // ========================= Part 2 =========================
        // I used a trick for handling data overrun
        if (counter == 25) {
            ADCSRA |= (1 << ADSC);                  // turn the ADC on after 25 round once
            counter = 0;
        }
        _delay_ms(5);
        counter++;
        // ========================= =========================

LDR:
        // ========================= Part 3 =========================
        // transmitting LDR sensor data to the slave
        // we are not using any int. in this part.
        cli();

        // ADC config for LDR
        adcConfig_LDR();

        // ADC read for LDR
        // divided to 7.65 bcz I scaled it to 100
        ldrValue = adc_read_LDR() / 7.65;           // Read ADC value from channel port 2 (connected to LDR)

        // transmitting
        PORTB &= ~(1 << PORTB4);                    // select slave
        SPDR = (int)(ldrValue) + 150;               // (+150) is for number categorizing for transmitting to slave
        // SPDR = 128;
        while(((SPSR >> SPIF) & 1) == 0);           // check if the data sending complete or not
        ignore = SPDR;

        _delay_ms(5);
        sei();
        _delay_ms(200);
        // ========================= =========================

    }
}


// ========== ISR for ADC ========== 
ISR(ADC_vect) {                                                 
    celsius = round(ADCW * 0.488);
    _delay_ms(10);                 
    PORTB &= ~(1 << PORTB4);                            // select slave
    SPDR = (int)(celsius);
    while(((SPSR >> SPIF) & 1) == 0);                   // check if the data sending complete or not
    ignore = SPDR;                        
}
// ===================================================

// ========== ISR for toggling int. ========== 
ISR(INT0_vect) {
    PORTD ^= (1 << 0);  
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
        KEYPAD_DDR |= (0x40 >> col);                // we shift to right each time to check which column // 0010 0000
        for (row = 0; row < 4; row++) {
            if (!(KEYPAD_PIN & (0x08 >> row))) {    // we shift to right each time to check which row      // 0000 0001
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
    DDRB |= (1 << DDB7) | (0 << DDB6) | (1 << DDB5) | (1 << DDB4);   // SCK, MISO, MOSI, SS_BAR
    PORTB |= (1 << PORT4);                          // select slave
    // config: local int. on, turn SPI on, MSB first, Master on, default sampling mode, clock: f_osc/128 
    SPCR = (1 << SPIE) | (1 << SPE) | (0 << DORD) | (1 << MSTR) | (0 << CPOL) | (0 << CPHA) | (1 << SPR1) | (1 << SPR0);  
    SPSR = (0 << SPI2X);  
}
// ===================================================

// ==================== ADC config for LM35 ====================
/* Description: 
conversion:
in atmega32 n = 10 (resolution)
Our step is single ended, then:
Max output = (2^n) - 1 = (2^10) - 1 = 1023
V_digital = (V_in * Max output) / V_ref
In atmega32 ===> Step = V_ref / Max output = 5000 / 1023 = 4.88 
Hence: V_digital = V_in * Step = ... * 4.88 
after that we should divide by 10 to convert the voltage value to the temperature
*/
void adcConfig_LM35() {
    ADMUX = (0 << REFS1) | (1 << REFS0);                // read max voltage from AVCC
    // enable interrupt, turn on ADC, auto trigger, prescale = 8
    ADCSRA |= (1<<ADEN) | (1<<ADIE) | (1 << ADATE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);      
    SFIOR = (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0); // free running
    // ADCSRA |= (1 << ADSC);                              // first conversion
}
// ===================================================

// ==================== Read ADC for LM35 ====================
int adc_read_LM35() {
    ADCSRA |= ((1 << ADSC) | (1 << ADIF));              // start conversion
    while ((ADCSRA & (1 << ADIF)) == 0);                  // wait till end of the conversion
    ADCSRA |= (1 << ADIF);                              // set the interrupt flag
    _delay_ms(1);   
    return ADCW;                                        // return ADC word
}
// ===================================================

// ==================== ADC config for LDR ====================
void adcConfig_LDR() {
    ADMUX = (1 << REFS0);                               // Reference voltage at AVCC
    ADCSRA = (1 << ADEN) | (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC with prescaler = 128
}
// ===================================================

// ==================== Read ADC for LDR ====================
int adc_read_LDR() {
    ADMUX = (ADMUX & 0xF0) | (1 << MUX1);     // Select ADC channel
    ADCSRA |= (1 << ADSC);                          // Start ADC conversion
    while (ADCSRA & (1 << ADSC));                    // Wait for conversion to complete
    return ADCW;
}
// ===================================================
