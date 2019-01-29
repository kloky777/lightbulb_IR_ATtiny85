/*
This program is created by Andrej Klochan
The program is controling the process of light bulb switching on and off, moreover, the charging of the battery is controlled by this program
The last update of this program: 29.1.2019
Added debug function
Changed battery charging conditions
The last changes made in this program: recharging battery condition, if the battery is not fully charged after 2.5 hours.
This changes are made in void charging() function
Add void checkRelayStuck() function - checks, whether relay is not stuck after charging.
Add system_sleep() in loop function
*/

#include <avr/wdt.h>
#include <tiny_IRremote.h>
#include <avr/sleep.h>
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

const int baterka = 0;
const int light = 1;
const int RECV_PIN = 2;  //RECV_PIN, pin = 2, physical pin 7 on ATTINY85
const int groundIR = 3;
long intervalsleep=5000;
long previoussleep=0;
volatile boolean f_wdt=1;
volatile boolean f_pcint0=1;
int counter=111;//this variable has to be set close to 113 in the start of the program, because of the charging function - checking, wether the charging of the battery has to be done
int bcounter=0;
int state=0;
boolean charged = false;    //assumption: battery is not charged above 4.1V at the start

IRrecv irrecv(RECV_PIN);
decode_results results;
#define UP  0xFF629D
#define DOWN 0xFFA857
#define NINE 0xFF5AA5


void setup(){

 // CPU Sleep Modes
 // SM2 SM1 SM0 Sleep Mode
 // 0    0  0 Idle
 // 0    0  1 ADC Noise Reduction
 // 0    1  0 Power-down
 // 0    1  1 Power-save
 // 1    0  0 Reserved
 // 1    0  1 Reserved
 // 1    1  0 Standby(1)
setup_watchdog(9);
pinMode(light, OUTPUT);  
pinMode(baterka, OUTPUT);
pinMode(groundIR, OUTPUT); 
digitalWrite(groundIR, LOW); 
irrecv.enableIRIn();
cbi(ADCSRA,ADEN); // Switch Analog to Digital converter OFF
sbi(GIMSK,PCIE); // Turn on Pin Change interrupt
sbi(PCMSK,PCINT2); // Which pins are affected by the interrupt
 
}

//****************************************************************
//****************************************************************
//****************************************************************
void loop()

{
  if(f_pcint0==1)
    {
      if (irrecv.decode(&results))  //this code runs only if some informations of IR are received
        {
          switch (results.value)
            {
               case UP:
               digitalWrite(light, (state) ? HIGH : LOW);
               state = !state;
               break;
               case DOWN:
               digitalWrite(baterka, HIGH);
               delay(5000);
               digitalWrite(baterka, LOW);
               break;
               case NINE:
               debug();
                      }
           irrecv.resume();
                             }
//------------------------------------------------------
     unsigned long currentMillis = millis();
     if(currentMillis - previoussleep > intervalsleep) 
       {
          previoussleep = currentMillis;
          f_pcint0=0;
          system_sleep();
                           }
                                 }
 if (f_wdt==1) 
   {  // wait for timed out watchdog / f_wdt is set when a watchdog timeout occurs
      f_wdt=0;
      if(counter>=113)  // if ==10 -> this will be true every 10x8 = 80seconds; set to 113 to get 113x8=904s = 15min 
         {
    
            charging();
            checkRelayStuck();
            counter = 0;
            system_sleep(); //27.12.2018 update
                          }
      else 
         {
            counter++;
            system_sleep();
                          }
                                     }

}



//****************************************************************
// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
  
set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
sleep_enable();
sleep_mode();                        // System sleeps here
sleep_disable();                     // System continues execution here when watchdog timed out
}

void charging()    //This part of program runs every 15minues
{
//------------------------------------------------------
 sbi(ADCSRA,ADEN);  // Switch Analog to Digital converter ON
//------------------------------------------------------
 int pin_baterka = digitalRead(0);
 int battery = analogRead(A2);
 if (battery <=645)  // If the battery has lower output voltage than 3V, starts charging
{
digitalWrite(baterka,HIGH);        // So the charging relay is switched on
bcounter=0;                        // And the bcounter is reseted
charged = false;        // battery is not charged

                          }

//------------------------------------------------------
if(bcounter >= 10 && pin_baterka==HIGH)        // 15minutes multiplied by ten is equal to two and a half of an hour
{
digitalWrite(baterka,LOW);   // after 2,5hours of charging, the charging relay is switched off
bcounter=0;
charged = true;           //this has to be true only if pin_baterka is HIGH
}
else if (bcounter >=10 && pin_baterka == LOW)
{
charged = false;
bcounter++;
}
else {
  bcounter++;
  }

if(charged == true && battery <= 810 && bcounter == 1) {
  charged = false;
  digitalWrite(baterka, HIGH);
  bcounter = 6;
  }
//------------------------------------------------------
cbi(ADCSRA,ADEN); // Switch Analog to Digital converter OFF
//------------------------------------------------------
}

void checkRelayStuck() {
  //------------------------------------------------------
 sbi(ADCSRA,ADEN);  // Switch Analog to Digital converter ON
//------------------------------------------------------
  
  int pin_baterka = digitalRead(0);
  int battery = analogRead(A2);
  if (battery>=840 && pin_baterka==LOW)    // This condition is fulfilled, if the charging relay is stuck, it means that AC charger charges the battery for a long time
{
digitalWrite(baterka,HIGH);                          // Hence, the charging relay is going to be switched on anyway
delay(2000);
digitalWrite(baterka,LOW);                            // and after two seconds off, in order to reset the relay
                              }
//------------------------------------------------------
cbi(ADCSRA,ADEN); // Switch Analog to Digital converter OFF
//------------------------------------------------------
  
  }



//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {

 byte bb;
 int ww;
 if (ii > 9 ) ii=9;
 bb=ii & 7;
 if (ii > 7) bb|= (1<<5);
 bb|= (1<<WDCE);
 ww=bb;
 

 MCUSR &= ~(1<<WDRF);
 // start timed sequence
 WDTCR |= (1<<WDCE) | (1<<WDE);
 // set new watchdog timeout value
 WDTCR = bb;
 WDTCR |= _BV(WDIE);

}
//****************************************************************
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) {
 f_wdt=1;  // set global flag
}
 ISR(PCINT0_vect) {
f_pcint0=1;  
}

void debug() {
  sbi(ADCSRA,ADEN);  // Switch Analog to Digital converter ON
  int battery = analogRead(A2);
  for(int i = 0; i < 9; i++) {
    if((battery - i) % 10 == 0) {
      
      digitalWrite(light, HIGH);
      delay(i * 1000);
      digitalWrite(light, LOW);
      delay(1000);
      battery = battery/10;
    }
  }
  
  for(int i = 0; i < 9; i++) {
    if((battery - i) % 10 == 0) {
      digitalWrite(light, HIGH);
      delay(i * 1000);
      digitalWrite(light, LOW);
      delay(1000);
      battery = battery/10;
    }
  }
  
  for(int i = 0; i < 9; i++) {
    if((battery - i) % 10 == 0) {
      digitalWrite(light, HIGH);
      delay(i * 1000);
      digitalWrite(light, LOW);
      delay(1000);
      }
  }
  
  
  cbi(ADCSRA,ADEN); // Switch Analog to Digital converter OFF
}
