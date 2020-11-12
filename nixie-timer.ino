#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 4096000UL
#include <util/delay.h>

#define DECIMAL_LAMP 7
#define UNARY_LAMP 6

#define INCREMENT 4
#define RESET 5

#define BLANK 15

#define BUZZER 2

void nixieTurnOn(int lampId) {
  int mask = (1 << lampId);
  PORTA &= ~mask; 
}

void nixieTurnOff(int lampId) {
  int mask = (1 << lampId);
  PORTA |= mask;  
}

void nixieSetNumber(int number) {
  if(number >= 10 || number < 0) {
    // turn off the tube
    number = 0x0f;
  }
  PORTA = (PORTA & ~0x0f) | number;
}

int blankifyZero(int number) {
  return number == 0 ? BLANK : number;
}

volatile int minutes = 0;
volatile int seconds = 0;

volatile int minutesToWait = 0;

volatile int beep = 0;
volatile int blinkPhase = 0;

volatile int timer0Counter = 0;

// TIMER0, frequency 20Hz
ISR(TIM0_COMPA_vect) {
  
  if(beep) {
    
    if(timer0Counter == 0 || timer0Counter == 10) {
      PORTB |= (1 << BUZZER);
    } else if(timer0Counter == 5 || timer0Counter == 15) {
      PORTB &= ~(1 << BUZZER);
    }
    
    if(timer0Counter % 10 == 0) {
      blinkPhase = !blinkPhase;
    }
    
    timer0Counter++;
    timer0Counter %= 40;
    
  } else {
    timer0Counter = 0;
    PORTB &= ~(1 << BUZZER);
  }
}

// TIMER1, frequency 1Hz
ISR(TIM1_COMPA_vect) {
  
  if(!(PINA & (1 << RESET))) {
    minutesToWait = 0;
    minutes = 0;
    seconds = 0;
    beep = 0;
  } else if(!(PINA & (1 << INCREMENT))) {
    minutesToWait++;
  } else if(minutesToWait > 0) {
    seconds++;
    if(seconds == 60) {
      minutes++;
      seconds = 0;
    }
    if(minutes == minutesToWait) {
      minutesToWait = 0;
      minutes = 0;
      seconds = 0;
      beep = 1;
    }
  }
}

int main() {

        // outputs PA0..3 & 6 & 7
        PORTA = 0xff;
        DDRA |= 0xcf;
        
        // beeper output PB2
        PORTB = 0x00;
        DDRB |= 0x04;
        
        // inputs
        DDRA &= ~(1 << RESET | 1 << INCREMENT);
        
        nixieTurnOff(DECIMAL_LAMP);
        nixieTurnOff(UNARY_LAMP);
        
        cli();

        TCCR1A = 0;
        TCCR1B = 0;
        TCCR1C = 0;
        TCNT1  = 0;
        
        TCCR0A = 0;
        TCCR0B = 0;
        TCNT0 = 0;
    
        OCR1A = 16000;
        TCCR1B = (1 << WGM12) | (4 << CS10);
        TIMSK1 |= (1 << OCIE1A);
        
        OCR0A = 200;
        TCCR0B = (1 << WGM02) | (5 << CS00);
        TIMSK0 |= (1 << OCIE1A);
        
        sei(); 
        
        while(1) {
          
          int decimal;
          int unary;
          
          if(beep) {
            if(blinkPhase) {
              decimal = 0;
              unary = 0;
            } else {
              decimal = BLANK;
              unary = BLANK;
            }
          } else {
            if(minutesToWait == 0 || minutesToWait - minutes == 0) {
              decimal = BLANK;
              unary = BLANK;
            } else if(minutesToWait - minutes == 1 && seconds < 60) {
              decimal = (60 - seconds) / 10 % 10;
              unary = (60 - seconds) % 10;
            } else {
              decimal = (minutesToWait - minutes) / 10 % 10;
              unary = (minutesToWait - minutes) % 10;
            }
          }
          
          nixieSetNumber(blankifyZero(decimal));
          _delay_ms(1);
          nixieTurnOn(DECIMAL_LAMP);
          _delay_ms(2);
          nixieTurnOff(DECIMAL_LAMP);
          _delay_ms(2);
          nixieSetNumber(unary);
          _delay_ms(1);
          nixieTurnOn(UNARY_LAMP);
          _delay_ms(2);
          nixieTurnOff(UNARY_LAMP);
          _delay_ms(2);
        }

        return 1;
}
