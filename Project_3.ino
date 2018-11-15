/* Trisha Ramdhoni
   Hand gesture controlled motor car
   Project 3
   Nov 3rd, 2018
*/

#include "USART.h";
#include <avr/io.h>;

uint16_t sensedADC = 0;
int mean = 0; // maybe initialize mean to first ADC value to prevent lag?
int N = 25; //size of sliding window
char msg[64]; // just to print things
int tiltAngle;

int main(void) {

  /*
     Configuring GPIO:
        - motor 1 control Pad 1, 2, 6 PD[1,2,6]
        - motor 2 control Pad 8, 9, 11 (PB[0:2])
        - PD 4 - slider switch input
  */
  DDRD = 0b11101111;
  PORTD = 0b00000010;
  DDRB = 0xFF;
  PORTB = 0b00000010;

  /* Configuring Timer/Counter0: v1 ms interrupts
     control the speed of rotation for motor 1
  */
  //TCCR0A = 0x83; // Fast PWM mode and set OC0A on compare match
  TCCR0B = 0x05; // prescale by 1024
  TIMSK0 = 0x02; // enable timer/counter 0, comp A

  /*
     Triggering button on Pad 4 PD4 to switch motors on and off
  */
  PCICR = 0x04; // enable PCINT2
  PCMSK2 = 0x10; // enable PCINT2 on Pad 4 (:20) and for push button on Pad 13 (:23)

  /* Configuring Timer/Counter0: v2 ms interrupts
    control the speed of rotation for motor 2
  */
  //TCCR2A = 0x83; // Fast PWM mode and set OC2A on compare match
  TCCR2B = 0x05; // prescale by 1024
  TIMSK2 = 0x02; // enable timer/counter 2, comp A

  /*
    Triggering an ADC for readings from pad A2
    (using the z axis from accelerometer)
  */
  ADMUX = 0b01000010; // pad A2, AVcc Ref and right aligned
  ADCSRA = 0xEF; // enabling ADC ISR and conversions
  ADCSRB = 0x05; // triggering a timer1 comp B
  DIDR0 = 0x3F; // disabling digital input from ADC0-5

  /*
    Configuring a Timer1/Compare Match B ISR to
    trigger ISR readings from pad A2 every 10 ms
  */
  TCCR1A = 2; // CTC mode
  TCCR1B = 0x05; // prescale by 1024
  TIMSK1 = 0x04; // Timer / Counter 1, Compare Match B
  OCR1BL = 156; // same as OCR0A?
  OCR1BH = 0;

  /* Configure serial bus to print debugging info */
  initUSART();

  /* Global interrupt enable */
  SREG |= 0x80;

  while (true) {
    uint16_t rx = getNumber();
    printString("\t");
    printHexByte( rx );
    printString("\n");
    ;
  }
  return 0;
}

ISR(ADC_vect) {
  sensedADC = ADCL;
  sensedADC |= (ADCH & 0x03) << 8;
  // the sliding mean is giving weird values so no
  // mean = mean * float(N - 1) / float(N) + float(sensedADC) / float(N);
  tiltAngle = ((int)sensedADC - 350) / -0.218;
//  sprintf(msg, "sensedADC: %d tiltAngle: %d \n", sensedADC, tiltAngle);
//  printString(msg);
}

// motor 1
ISR(TIMER0_COMPA_vect) {
  if (tiltAngle > 0) {
    OCR0A = 2 * tiltAngle + 100;
    OCR2A = 3 * tiltAngle + 100;
  }
  else {
    OCR0A = 3 * abs(tiltAngle) + 100;
    OCR2A = 2 * abs(tiltAngle) + 50; /*had to cheat here cause the second motor (attached to 0CR0A)
                                      seems a little faulty / slower than the other at the same speeds*/
  }
}

// motor 2
ISR(TIMER2_COMPA_vect) {

}

// PCINT2 to check if switch is on or off
ISR(PCINT2_vect) {

  if (PIND & 0b00010000) {
    TCCR0A = 0x00; // switches motor off
    TCCR2A = 0x00;
  }
  // switches motor on my triggering timers 0 and 2
  else {
    TCCR0A = 0x83; // Fast PWM mode and set OC0A on compare match
    TCCR2A = 0x83;
  }
}
