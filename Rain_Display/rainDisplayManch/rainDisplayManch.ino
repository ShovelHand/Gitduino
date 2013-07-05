#include <MANCHESTER.h>  //this manchester encoding library is available at https://github.com/mchr3k/arduino-libs-manchester
#define rxPin 2

/* this is the half of the rain sensing installation where when rain is sensed by the other half, serial transmissions decoded by the radio transceiver causes a rain animation by leds chasing
 vertically on a 7x8 common cathode led matrix. The max7219 IC was used to control the matrix.  Not all of the code is mine and a lot of the ground work came from a code example on the ardiuno playground 
 (http://www.arduino.cc/playground/LEDMatrix/Max7219).
 Alex C, July 17, 2012
 **Update Aug. 24, 2012**  This all worked well except that the rf receiver outputs noise when it is not constantly getting data from the transmitter.  Therefore I had to rethink the code so that it's based on bytes being
 sent rather than simply detecting radio pulses.  This created power consumption issues that will need to be ironed out.
 **Nov. 28, 2012**  It works fine now and does exactly what it should do and looks great.  Now I'm just working to make it consume as little power as possible.
 While initially I was using virtualWire for my serial transmissions, I eventually switched to manchester encoding as it was much easier to setup an attiny85 as the transmitter.  Using a atmega for this would have been 
 total overkill.
 */

//#include <avr/sleep.h>    //these libraries were to be included for power saving considerations
//#include <avr/power.h>

//the following outputs drive the max7219 
int dataIn = 5; //serial data 
int load = 3;   //latch 
int clock = 4;  //clock signal 

// define max7219 registers see datasheet http://www.adafruit.com/datasheets/MAX7219.pdf for specifics
byte max7219_reg_decodeMode  = 0x09;
byte max7219_reg_intensity   = 0x0a;
byte max7219_reg_scanLimit   = 0x0b;
byte max7219_reg_shutdown    = 0x0c;
byte max7219_reg_displayTest = 0x0f;

int columns[] = {1,2,4,8,16,32,64,128}; //an array of column indices, easier this way since columns are called by 2^n


//controls the bitwise specifics of sending serial data to the max7219. I did not write this method
void putByte(byte data) {
  byte i = 8;                    //start at most significant bit
  byte mask;
  while(i > 0) {
    mask = 0x01 << (i - 1);      // get bitmask, logical shift left
    digitalWrite( clock, LOW);   // tick
    if (data & mask){            // choose bit, data and mask both = 1?
      digitalWrite(dataIn, HIGH);// send 1
    }
    else{
      digitalWrite(dataIn, LOW); // send 0
    }
    digitalWrite(clock, HIGH);   // tock
    --i;                         // move to lesser bit
  }
}
//I did not write this method
//maxSingle is the "easy"  function to use for a single max7219
void maxSingle(byte reg, byte col) {    
  digitalWrite(load, LOW);       // begin.  Latch low    
  putByte(reg);                  // specify register
  putByte(col);//((data & 0x01) * 256) + data >> 1); // put data  
  digitalWrite(load,HIGH);      //close latch
}


//this method causes the vertical led chasing that is the rain animation
void drop(){
  //sleep_disable();                         //wake up the processor
  maxSingle(max7219_reg_shutdown, 0x01);   //wake up the max7219
  byte e;                                  //used to help with nop delays
  byte row = 1;                            //horizontal row always begins at 1, the top.
  byte col = columns[random(1,8)];         //the vertical column is chosen by picking a random index of column array.  I don't know why, but it didn't like starting at 0 
  while (row <= 8){                        //coloumn is 8 leds deep, so end here
    int last = row -1;
    maxSingle(row, col);                   //light the led
    e = 2;  
    while(e>0){
    for(int i = 0; i <1500; i++){
      //only using the assembly command "no op" because this was originally taking place during an ISR, and delay() doesn't work well during interrupts.
    __asm__("nop\n\t");        //also, I am using the 1 mhz internal clock, but the compiler thinks I'm using a 8 mhz arduino LilyPad, so nop let's me tailor my delays more directly.
    }
    e--;
    }
    //no op for a cycle, not a specifically timed delay
    maxSingle(last, 0);            //previous led stays on briefly for a nice blended trasition
    e = 2;  
    while(e>0){
    for(int i = 0; i <1500; i++){
    __asm__("nop\n\t");
    }
    e--;
    }
    if (row == 8){                //if it's the bottom one...
       e = 5;  
    while(e>0){
    for(int i = 0; i <1500; i++){
    __asm__("nop\n\t");
    }
    e--;
    }
    }
    row++;
  }
}
 /* I will have to think of some better ways to minimize power consumption while still listening for broadcasts
void enterSleep(void){
 Setup pin2 as an interrupt and attach handler. 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
}
*/
/* This sketch actually never calls this function.  The idea was that if no serial data was being passed by the rain sensor, then randome leds would light up to add visual interest.
I kind of like this idea, but the hardware runs off of a 9v battery, so it's ignored in favour of power saving.  It's here if you want it.*/
void lightSingle(){
  int row = random(9);
  int col = columns[random(0,8)];  //random is a range from the first argument up to, but not including the second argument, so we get values from 0 through 7.
  maxSingle(row, col);
}


void setup () {
MANRX_SetRxPin(rxPin);  //set the arduino digital pin for receive. default 4.
MANRX_SetupReceive();
MANRX_BeginReceive();

  pinMode(dataIn, OUTPUT);
  pinMode(clock,  OUTPUT);
  pinMode(load,   OUTPUT);

  // turn OFF analog parts to save a little power.  
  ACSR = ACSR & 0b11110111 ; // clearing ACIE prevent analog interupts happening during next command
  ACSR = ACSR | 0b10000000 ; // set ACD bit powers off analog comparator
  ADCSRA = ADCSRA & 0b01111111 ;  // clearing ADEN turns off analog digital converter
  ADMUX &= B00111111;  // Comparator uses AREF/GND and not internally generated references

  //initiation of the max 7219,  I did not write this bit
  maxSingle(max7219_reg_scanLimit, 0x07);      
  maxSingle(max7219_reg_decodeMode, 0x00);  // using an led matrix (not digits)
  maxSingle(max7219_reg_shutdown, 0x01);    // not in shutdown mode
  maxSingle(max7219_reg_displayTest, 0x00); // no display test.  If this is passed th evalue 0x01, it forces every led to be on. 
  int e = 0; 
  for (e=1; e<=8; e++) {    // empty registers, turn all LEDs off
    maxSingle(e,0);
  }
  maxSingle(max7219_reg_intensity, 0x0f & 0x0f);    // the first 0x0f is the value you can set
  // range: 0x00 to 0x0f 
}
void loop () {    
    //are we receiving a broadcast from the tx component?
    if (MANRX_ReceiveComplete()){
      unsigned int data = MANRX_GetMessage();
      MANRX_BeginReceive();
      if (data == 1234){  //was it the broadcast we were looking for?  (the value 1234 is completely arbitrary, you can send/receive any value you want.)
      maxSingle(max7219_reg_shutdown, 0x01);  //make sure the matrix driver is on
      drop();                        //do the led chase sequence.
      maxSingle(8,0);              //turn off last led so it doesn't light up next interrupt 
  }
  else{
    maxSingle(max7219_reg_shutdown, 0x00); //turn off max when we're not using it to save power
}
    }
  
}




