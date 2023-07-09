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
#define ToggleNum  15
char ignore;
int key = 0x00;
int encryptionKey = 0x99;
int forbidden = 23;
// === ADC ===
float celsius = 27.00;
float prev_celsius = 27.00;
char temperature[4];
// === Motors ===
int counter = 0;                                    // trick for handling data overrun 
int counter_LDR = 0;                                // trick for handling data overrun 
int turn = 2;
// === Lightening ===
float ldrValue;
// === Password ===
int isLogin = 0;
// ===================================================


// ==================== Functions ====================
int keypadRead();
void transmitDataConfig();
void transmitData(int);
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

    // === toggle ===
    // led check
    DDRD |= (1 << 0);
    PORTD |= (0 << 0);
    
    GICR |= (1 << INT0);                            // set INT0
    MCUCR = 0x03;                                   // MCUCR = 0000 1111 => for rising edge
    
    // ADCSRA |= (1 << ADSC);
    sei();                                          // global int. enable

    while (1) {
        _delay_ms(1);
        // === set login state ===
        if (((PIND >> PIND3) & 1)) {
            isLogin = 1;
            DDRC |= 0x80;
            PORTC |= 0x80;
        }
        _delay_ms(1);

        if (isLogin && (turn == 2)) goto Part2; 
        // if (isLogin && (turn == 3)) goto Part3; 
        // if (isLogin == 1) goto Part3;
        _delay_ms(1);

        // ========================= Part 1 ========================= 
        key = keypadRead();
        // _delay_ms(300);
        // if (key == forbidden) goto ADC_Part;
        if (key == forbidden) continue;

        // === some other cases ===
        // send key = 10 for submitting, '*'
        // zero
        if (key == 11) {
            key = 0;
        }
        // send key = 12 for deleting, '#'

        // === transmitData ===
        PORTB &= ~(1 << PORTB4);                    // select slave

        // == Encrypting data == 
        /* Description: for encrypting data I choose to "xor" the data
        before transmitting with a number as key of encryption. */
        key ^= encryptionKey;
        SPDR = key;
        while(((SPSR >> SPIF) & 1) == 0);           // check if the data sending complete or not
        ignore = SPDR;

        _delay_ms(5);
        continue;
        // ========================= =========================


Part2:
        // ========================= Part 2 =========================
        adcConfig_LM35();
        // I used a trick for handling data overrun
        if (counter > 100) {
            ADCSRA |= (1 << ADSC);                  // turn the ADC on after 25 round once
            counter = 0;
            turn++;
        }
        _delay_ms(5);
        counter++;
        _delay_ms(5);
        // ========================= =========================

continue;
Part3:
        // ========================= Part 3 =========================
        // transmitting LDR sensor data to the slave
        // we are not using any int. in this part.
        cli();

        // ADC config for LDR
        adcConfig_LDR();

        // ADC read for LDR
        // divided to 7.65 bcz I wanna scale it to 100
        ldrValue = adc_read_LDR() / 7.65;               // Read ADC value from channel port 2 (connected to LDR)

        // transmitting
        if (counter_LDR > 100) {
            PORTB &= ~(1 << PORTB4);                    // select slave
            SPDR = (int)(ldrValue) + 150;               // (+150) is for number categorizing for transmitting to slave
            while(((SPSR >> SPIF) & 1) == 0);           // check if the data sending complete or not
            ignore = SPDR;
            counter_LDR = 0;
            turn--;
        }
        counter_LDR++;

        _delay_ms(5);
        sei();
        _delay_ms(5);
        // ========================= =========================
    }
}


// ========== ISR for ADC_1 ========== 
ISR(ADC_vect) {
    celsius = round(ADCW * 0.488);
    _delay_ms(10);                 
    PORTB &= ~(1 << PORTB4);                        // select slave
    SPDR = (int)(celsius) + 40;
    while(((SPSR >> SPIF) & 1) == 0);               // check if the data sending complete or not
    ignore = SPDR;   
        // ADCSRA |= (1 << ADSC);      
}
// ===================================================

// ========== ISR for toggling int0. ========== 
ISR(INT0_vect) {
    PORTD ^= (1 << 0);  
    PORTB &= ~(1 << PORTB4);                        // select slave
    // == Encryption
    int temp = ToggleNum;
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
    return forbidden;                               // no key pressed
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
    // PORTB &= ~(1 << PORTB4);                    // select slave
}
// ===================================================

// ==================== transmit data ====================
void transmitData(int value) {
    PORTB &= ~(1 << PORTB4);                    // select slave
    SPDR = value;               // (+150) is for number categorizing for transmitting to slave
    // while(((SPSR >> SPIF) & 1) == 0);           // check if the data sending complete or not
    _delay_ms(100);
    ignore = SPDR;
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
    ADMUX = (0 << REFS1) | (1 << REFS0);            // read max voltage from AVCC
    // enable interrupt, turn on ADC, auto trigger, prescale = 8
    ADCSRA |= (1<<ADEN) | (1<<ADIE) | (1 << ADATE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);      
    SFIOR = (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0); // free running
}
// ===================================================

// ==================== Read ADC for LM35 ====================
// not used!
int adc_read_LM35() {
    ADCSRA |= ((1 << ADSC) | (1 << ADIF));          // start conversion
    while ((ADCSRA & (1 << ADIF)) == 0);            // wait till end of the conversion
    ADCSRA |= (1 << ADIF);                          // set the interrupt flag
    _delay_ms(1);   
    return ADCW;                                    // return ADC word
}
// ===================================================

// ==================== ADC config for LDR ====================
void adcConfig_LDR() {
    ADMUX = (1 << REFS0);                           // Reference voltage at AVCC
    ADCSRA = (1 << ADEN) | (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC with prescaler = 128
}
// ===================================================

// ==================== Read ADC for LDR ====================
int adc_read_LDR() {
    ADMUX = (ADMUX & 0xF0) | (1 << MUX1);           // Select ADC channel
    ADCSRA |= (1 << ADIE) | (1 << ADSC);                          // Start ADC conversion
    while (ADCSRA & (1 << ADSC));                   // Wait for conversion to complete
    return ADCW;
}
// ===================================================
