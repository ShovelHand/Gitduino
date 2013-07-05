/*the purpose of this code is to use an attiny to generate a manchester encoded RF transmission whenever the chip completes an 
analog to digital conversion.  A piezo element attached to some surface is the source of the analog input.
Manchester library available here https://github.com/mchr3k/arduino-libs-manchester.git*/


#include <MANCHESTER.h>
#define txPin 2
#define inPin 4   //pin that receives the analog input from the piezo element


void setup() {

pinMode(inPin, INPUT);

//attachInterrupt (2, trans, RISING);  taken care of by register settings
MANCHESTER.SetTxPin(txPin);
// ADC interrupt considerations
 ADMUX =
         (1 << ADLAR) |     // left shift result
         /*this code block sets the aref to internal 2.56v w/o external acapictor needed
         (1 << REFS2)|
         (1 << REFS1)|
         */
            (0 << REFS1) |     // Sets ref. voltage to VCC, bit 1
            (0 << REFS0) |     // Sets ref. voltage to VCC, bit 0
            (0 << MUX3)  |     // use ADC2 for input (PB4), MUX bit 3
            (0 << MUX2)  |     // use ADC2 for input (PB4), MUX bit 2
            (1 << MUX1)  |     // use ADC2 for input (PB4), MUX bit 1
            (0 << MUX0);       // use ADC2 for input (PB4), MUX bit 0
          
   ADCSRA =
        (1 << ADEN)  |     // Enable ADC
        (1 << ADATE) |     // auto trigger enable
        (1 << ADIE)  |     // enable ADC interrupt
        (1 << ADPS0) |     // Prescaler = 8
        (1 << ADPS1);      //    - 125KHz with 1MHz clock

    ADCSRB = 0;                  // free running mode
    sei();
    ADCSRA |= (1 << ADSC); // start conversions
 
}

void loop() {
//trans();

}

void trans(){
  unsigned int data = 1234;
  MANCHESTER.Transmit(data);
}

ISR(ADC_vect){

  //could use an if loop to set a threshold level required for a transmission
  if(ADCH > 10){
trans();
//delay(30); // a brief delay to give the adc a rest.
  }
}

