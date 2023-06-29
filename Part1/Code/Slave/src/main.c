// Ali Maher _ 9932113
// Project Part 1 _ Security
// main code of "slave" atmega32

#include <avr/io.h>
#include <util/delay.h>
#include <LCD.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>

// ==================== Definitions ====================
int encryptionKey = 0x99;
int numberOfInputs = 0;
volatile int toggle = 1;
int prev_toggle = 1;
char receivedData;
char* hardCodedPassword = "2";
char enteredPassword[40];
// ===================================================


// ==================== Functions ====================
void sayHello();
void receiveDataConfig();
char receiveData();
void checkPassword(int);
void delete();
void checkTogglePassword();
// ===================================================


int main(void) {
    // Communication with master atmega32
    receiveDataConfig();

    // LCD setting
    DDRC = 0xFF;
    DDRD = 0x07;
    init_LCD();

    // welcome showing
    // sayHello();

    // toggle
    char temp[2];    

    while (1) {
        // check password status show in '*' or not
        checkTogglePassword();
        
        // read data
        receivedData = receiveData();
        numberOfInputs++;

        // == Decryption ==
        receivedData ^= encryptionKey;
        // ==

        // toggle password
        _delay_ms(5);
        if (receivedData == 'T') {
            itoa(toggle, temp, 10); // test
            LCD_write(temp[0]);     // test
            toggle ^= 1;
            numberOfInputs--;
            itoa(toggle, temp, 10); // test
            LCD_write(temp[0]);     // test
            continue;
        }
        prev_toggle = toggle;

        // check password
        if (receivedData == '*') { 
            checkPassword(strcmp(enteredPassword, hardCodedPassword));
            numberOfInputs--;
            continue; 
        }
        // ================================

        // delete
        if (receivedData == '#') {
            numberOfInputs--;
            delete();
            continue;
        }
        // ================================

        // save accepted number in our enteredPassword buffer
        enteredPassword[numberOfInputs - 1] = receivedData; 

        // display on LCD
        LCD_cmd(0x0F);                              // make blinking cursor
        _delay_ms(10);
        if (toggle == 1) LCD_write('*');
        else if (toggle == 0) LCD_write(receivedData);
        _delay_ms(10);
        LCD_cmd(0x0C);                              // turn off cursor (no blinking)

        _delay_ms(5);
    }
}


// ==================== Say Hello ====================
void sayHello() {
    unsigned char i, Hello[6] = "Hello!",
    welcomeTxt1[37] = "Welcome to the most secure CPS system",
    welcomeTxt2[35] = "designed for MicroProcessor course.", 
    me[30] = "Student: Ali Maher :]  9932113", 
    Dr[18] = "Professor: Dr.Raji";

    LCD_cmd(0x0F);                              // make blinking cursor
    _delay_ms(200);
    LCD_cmd(0x14);                              // move cursor to the right

    for (i = 0; i < 6; i++) {
        LCD_write(Hello[i]);
        _delay_ms(5);
    }

    _delay_ms(200);
    LCD_cmd(0x01);                              // clear screen
    LCD_cmd(0x14);

    for (i = 0; i < 37; i++) {
        LCD_write(welcomeTxt1[i]);
        _delay_ms(5);
    }

    _delay_ms(200);
    LCD_cmd(0x01);                              // clear screen
    LCD_cmd(0x14);

    for (i = 0; i < 35; i++) {
        LCD_write(welcomeTxt2[i]);
        _delay_ms(5);
    }

    _delay_ms(200);
    LCD_cmd(0x01);                              // clear screen
    LCD_cmd(0x14);

    for (i = 0; i < 30; i++) {
        LCD_write(me[i]);
        _delay_ms(5);
    }

    _delay_ms(200);
    LCD_cmd(0x01);                              // clear screen
    LCD_cmd(0x14);

    for (i = 0; i < 18; i++) {
        LCD_write(Dr[i]);
        _delay_ms(5);
    }

    LCD_cmd(0x0C);                              // turn off cursor (no blinking)
    _delay_ms(500);
    LCD_cmd(0x01); // clear screen
}
// ===================================================

// ==================== Receive Data Config ====================
/* Description: 
SPI initialization
SPI Type: Slave
SPI Clock Rate: 8MHz / 128 = 62.5 kHz
SPI Clock Phase: Cycle Half
SPI Clock Polarity: Low
SPI Data Order: MSB First    
*/
void receiveDataConfig() {
    DDRB = (0<<DDB7) | (1<<DDB6) | (0<<DDB5) | (0<<DDB4);
    // config: turn SPI on, MSB first, Master off, default sampling mode, clock: f_osc/128 
    SPCR = (1<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (1<<SPR1) | (1<<SPR0);
    SPSR = (0<<SPI2X);
}
// ===================================================

// ==================== Receive Data ====================
char receiveData() {
    SPDR = '0';
    while (((SPSR >> SPIF) & 1) == 0);
    return SPDR;
}
// ===================================================

// ==================== Check Password ====================
void checkPassword(int status) {
    unsigned char i,
    CorrectPassMSG[17] = "Access is granted", 
    WrongPassMSG[14] = "Wrong password";
    // correct password
    LCD_cmd(0x01);                              // clear screen
    LCD_cmd(0x0F);                              // make blinking cursor
    if (status == 0) {
        for (i = 0; i < 17; i++) {
            LCD_write(CorrectPassMSG[i]);
            _delay_ms(5);
        }
        // do sth ...
    } else {    // wrong password 
        for (i = 0; i < 14; i++) {
            LCD_write(WrongPassMSG[i]);
            _delay_ms(5);
        }
        // prepare for getting new password from user
        // 1. set the counter to zero
        // 2. empty the buffer
        numberOfInputs = 1;
        memset(enteredPassword, '\0', sizeof(enteredPassword));
    }
    LCD_cmd(0x0C);                              // turn off cursor (no blinking)
    _delay_ms(300);
    LCD_cmd(0x01);                              // clear screen
}
// ===================================================

// ==================== Delete ====================
void delete() {
    // change on the buffer
    enteredPassword[numberOfInputs -  1] = '\0';
    numberOfInputs--;
    // change on displayer
    LCD_cmd(0x0C);                              // turn off cursor (no blinking)
    LCD_cmd(0x10);                              // move cursor to the left
    LCD_write(' ');
    LCD_cmd(0x10);                              // move cursor to the left
}
// ===================================================

// ==================== check Toggle status ====================
void checkTogglePassword() {
    unsigned char i;
    if (prev_toggle != toggle) {
        LCD_cmd(0x01);                          // clear screen
        LCD_cmd(0x0F);                          // make blinking cursor
        if (toggle == 0) {
            for (i = 0; i < numberOfInputs; i++) {
                LCD_write(enteredPassword[i]);
            }
        }
        else if (toggle == 1) {
            for (i = 0; i < numberOfInputs; i++) {
                LCD_write('*');
            }
        }
    }
} 
// ===================================================
