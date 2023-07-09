// Ali Maher _ 9932113
// CPS Project of MP
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
uint32_t receivedData;
// Password
char* hardCodedPassword = "2";
char enteredPassword[40];
int isLogin = 0;
// ADC
int celsius;
// motors
double coolerDutyCycle = 50;
double heaterDutyCycle = 100;
// lightening
double lightningMonitoringDutyCycle = 0; 
float ldrValue;
// ===================================================


// ==================== Functions ====================
void sayHello();
void receiveDataConfig();
int receiveData();
void checkPassword(int);
void delete();
void checkTogglePassword();
void temperatureControlTimerCounterConfig();
void LDRTimerCounterConfig();
// ===================================================


int main(void) {
    // LCD setting
    DDRC = 0xFF;
    DDRD = 0x07;
    init_LCD();

    // Timer counter 2 setting for LDR
    LDRTimerCounterConfig();

    // Timer counter 1 setting of LM35
    temperatureControlTimerCounterConfig();

    // Communication with master atmega32
    receiveDataConfig();


    // welcome showing
    // sayHello();

    // toggle
    char temp[2];

    sei();    

    while (1) {
        // ==================== Part1 ====================
        // check password status show in '*' or not
        checkTogglePassword();

        // read data
        receivedData = receiveData();
        if (isLogin && receivedData >= 40 && receivedData <=145)  goto Part2;
        if (isLogin && receivedData >= 150) goto Part3;
        // if (isLogin) goto Part3;
        numberOfInputs++;

        // == Decryption ==
        receivedData ^= encryptionKey;
        // ==

        // toggle password
        _delay_ms(5);
        if (receivedData == 15) {
            itoa(toggle, temp, 10); // test
            LCD_write(temp[0]);     // test
            toggle ^= 1;
            numberOfInputs--;
            itoa(toggle, temp, 10); // test
            LCD_write(temp[0]);     // test
            continue;
        }

        // check password
        // submitting password (10 ===> '*')
        if (receivedData == 10) { 
            checkPassword(strcmp(enteredPassword, hardCodedPassword));
            numberOfInputs--;
            continue; 
        }
        // ================================

        // delete
        // deleting (12 ===> '#')
        if (receivedData == 12) {
            numberOfInputs--;
            delete();
            continue;
        }
        // ================================

        // save accepted number in our enteredPassword buffer
        enteredPassword[numberOfInputs - 1] = (char)(receivedData + 48); 

        // display on LCD
        LCD_cmd(0x0F);                              // make blinking cursor
        _delay_ms(10);
        if (toggle == 1) LCD_write('*');
        else if (toggle == 0) LCD_write((char)(receivedData + 48));
        _delay_ms(10);
        LCD_cmd(0x0C);                              // turn off cursor (no blinking)

        _delay_ms(5);

        continue;
        // ========== ========== ========== ==========



Part2:
        // ==================== Part2 ====================
        celsius = receivedData - 40;

        // using a self-made decoder :)
        // cooler (motor)
        if (celsius >= 25 && celsius <= 55) {
            coolerDutyCycle = ceil(((int)celsius - 25) / 5) * 10 + 50;
            PORTB &= 0xFC;
            PORTB |= (0 << PORTB0) | (0 << PORTB1);
            TCCR0 |= (1 << COM01);                  // clear Oc0 on compare match
            // goto Part3;
            continue;            
        } 
        // heater (motor)
        else if (celsius >= 3 && celsius <= 20) {
            heaterDutyCycle = 100 - ceil(((int)celsius - 25) / 5) * 25;
            PORTB &= 0xFC;
            PORTB |= (1 << PORTB0) | (0 << PORTB1);
            TCCR0 |= (1 << COM01);                  // clear Oc0 on compare match
            // goto Part3; 
            continue;           
        } 
        // temperature: 20 - 25 
        else if (celsius > 20 && celsius < 25) {
            coolerDutyCycle = 0;
            PORTB &= 0xFC;
            PORTB |= (0 << PORTB0) | (1 << PORTB1);
            TCCR0 &= 0xCF;                         
            // goto Part3;
            continue;            
        }
        // temperature less than 0 (0 - 3) (Blue Blink LED)
        else if (celsius >= 0 && celsius < 3) {
            coolerDutyCycle = 0;
            PORTB &= 0xFC;
            PORTB |= (1 << PORTB0) | (1 << PORTB1);
            _delay_ms(70);
            PORTA |= (1 << PORTA0);
            _delay_ms(70);
            PORTA &= 0xFE;
            // goto Part3;
            continue;            
        }
        // temperature greater than 55 (Red Blink LED)
        else if (celsius > 55) {
            coolerDutyCycle = 0;
            PORTB &= 0xFC;
            PORTB |= (1 << PORTB0) | (1 << PORTB1);
            _delay_ms(70);
            PORTA |= (1 << PORTA1);
            _delay_ms(70);
            PORTA &= 0xFD;
            // goto Part3; 
            continue;           
        }
        // ========== ========== ========== ==========



Part3:
// continue;     
        // ==================== Part3 ====================
        // receiving data
        if (receivedData >= 150) {
            numberOfInputs--;
            ldrValue = (receivedData) - 150;
        }

        // setting duty cycle
        // 75 - 100 
        if (ldrValue >= 75) {
            // LCD_write('1');
            lightningMonitoringDutyCycle = 25;
            // _delay_ms(5);
        }
        // 50 - 75
        else if (ldrValue >= 50) {
            // LCD_write('2');
            lightningMonitoringDutyCycle = 50;
            // _delay_ms(5);
        }
        // 25 - 50
        else if (ldrValue >= 25) {
            // LCD_write('3');
            lightningMonitoringDutyCycle = 75;
            // _delay_ms(5);
        }
        // 0 - 25
        else if (ldrValue >= 0) {
            // LCD_write('4');
            lightningMonitoringDutyCycle = 100;
            // _delay_ms(5);
        }
        OCR2 = (lightningMonitoringDutyCycle / 100) * 255; // duty cycle

        _delay_ms(5);

        continue;
        // ========== ========== ========== ==========
    }
}


// =============== ISR of TC0 OVF for Temperature =============== 
ISR (TIMER0_OVF_vect) {
  // === cooler ===
  if (celsius >= 25 && celsius <= 55) {
    coolerDutyCycle = ceil(((int)celsius - 25) / 5) * 10 + 50;
    if (coolerDutyCycle > 100) coolerDutyCycle = 100;
    OCR0 = (coolerDutyCycle / 100) * 255;
   }
  // === heater ===
  else if (celsius >= 3 && celsius <= 20) {
    heaterDutyCycle = 100 - ceil(((int)celsius - 25) / 5) * 25;
    if (heaterDutyCycle < 0) heaterDutyCycle = 0;
    OCR0 = (heaterDutyCycle / 100) * 255;
  }
}
// ===================================================

// =============== ISR of TC2 OVF for lightening =============== 
ISR (TIMER2_OVF_vect) {
  OCR2 = (lightningMonitoringDutyCycle / 100) * 255; // duty cycle
}
// ===================================================

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
    DDRB |= (0<<DDB7) | (1<<DDB6) | (0<<DDB5) | (0<<DDB4);
    // config: turn SPI on, MSB first, Master off, default sampling mode, clock: f_osc/128 
    SPCR = (1<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (1<<SPR1) | (1<<SPR0);
    SPSR = (0<<SPI2X);
}
// ===================================================

// ==================== Receive Data ====================
int receiveData() {
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
            // _delay_ms(2);
        }
        // sending loginSign to master
        DDRA |= (1 << PORT7);
        PORTA |= (1 << PORTA7);
        _delay_ms(100);
        DDRA &= 0x7F;
        PORTA &= 0x7F;
        // 
        isLogin  = 1;                            // set login status
    } else {    // wrong password 
        for (i = 0; i < 14; i++) {
            LCD_write(WrongPassMSG[i]);
            _delay_ms(2);
        }
        // prepare for getting new password from user
        // 1. set the counter to zero
        // 2. empty the buffer
        numberOfInputs = 1;
        memset(enteredPassword, '\0', sizeof(enteredPassword));
    }
    LCD_cmd(0x0C);                              // turn off cursor (no blinking)
    _delay_ms(200);
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
        prev_toggle = toggle;
    }
} 
// ===================================================

// ==================== Temperature control Timer Counter config ====================
void temperatureControlTimerCounterConfig() {
    DDRB |= (1 << PORTB0) | (1 << PORTB1) | (1 << PORTB3);
    TCCR0 |= (1 << WGM01) | (1 << WGM00);           // fast pwm
    TCCR0 |= (1 << CS01);                           // 1/8 clock prescaling
    TIMSK |= (1 << TOIE0);                          // over flow int. enable
    DDRA |= (1 << PORTA1) | (1 << PORTA0);          // LED port for check temperature status
}
// ===================================================

// ==================== LDR Timer Counter config ====================
void LDRTimerCounterConfig() {
    // DDRD &= 0x7F;
    DDRD |= (1 << PORTD7);
    TCCR2 = 0x00;
    TCCR2 |= (1 << WGM21) | (1 << WGM20);           // fast pwm
    TCCR2 |= (1 << COM21);                          // clear Oc0 on compare match
    TCCR2 |= (1 << CS21);                           // 1/8 clock prescaling
    OCR2 = (lightningMonitoringDutyCycle / 100) * 255;
    TIMSK |= (1 << TOIE2);                          // over flow int. enable
}
// ===================================================
