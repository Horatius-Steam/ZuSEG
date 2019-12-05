
//atmega32u4
//Board Arduino Leonardo
//Sketch to use a dial as input for number
//and 10 keys for special charakters
//plus a switch for CR
//
//Original code from:
//Dial                    >> https://github.com/antonmeyer/WaehlscheibeHID
//Key entry               >> https://janbpunkt.de/2017/10/21/ezeebox/
//German Keyboard library >> https://github.com/MichaelDworkin/Arduino-Leonardo-USB-Keyboard-Deutsch-library
// 

#include "TimerOne.h"
#include <KeyboardDE.h> //using the library made by Michael Dworkin instead
//#include <Keyboard.h> has some problems with German keyboard drivers.
//definition of pins for keys
//Pin 9 & 10 reserviert fuer Waehlscheibe
int keypadPins[] = {2, 3, 4, 5, 6, 7, 8, 11, 12, 13}; //Pins definieren, an denen Taster hängen 

//length of the level before valid - reflect the measurement period
static const byte samplerate = 5; // ms
static const byte lpLIMIT = 20 / samplerate; //detection limit impulse duration low
static const byte hpLIMIT = 40 / samplerate; //detection limit impluse duration high

volatile byte highpulse =0; //count nsi period high
volatile unsigned int lowpulse = 0; //count nsi period low
volatile byte iwvcnt =0; //counts impulse
byte iwv_old = 0; //temp var

static const byte NSApin = 9;  // NSA switch is low when the dail disk is moved
static const byte NSIpin = 10; // NSI when rotating back, nsi generates the impulses

unsigned short nsa_low_cnt =0;
unsigned short nsa_high_cnt =0;
boolean t1_started = false;
void nsi_cnt_ISR() { //called every ms
	
	
	if (!(digitalRead(NSApin))) { //just for safety ensure NSA is closed

		if (!digitalRead(NSIpin)) {
			if (lowpulse++ > lpLIMIT) {
				highpulse = 0; //reset the last high pulse
			}
		}
		else {
			if (highpulse++ > hpLIMIT) {
				if (lowpulse > lpLIMIT) iwvcnt++;
				lowpulse=0; //state changed to high, waiting for the next falling slope
			}
		}
	}
	
}

void setup()
{
  //Dial
  //Set all dial pins to input and high
	pinMode(NSApin, INPUT);
	digitalWrite(NSApin, HIGH);
	pinMode(NSIpin, INPUT);
	digitalWrite(NSIpin, HIGH);
  
  //Key entry
  //Set all key pins to input and high
     for (int i=0; i<10; i++) {     
      pinMode(keypadPins[i], INPUT);     
      digitalWrite(keypadPins[i], HIGH);   
   }
   //Set A0 as digital in for ENTER
   pinMode(A0, INPUT_PULLUP);
   //Set A1 as digital in for TAB
   //pinMode(A1, INPUT_PULLUP); //not used yet
   
	//Serial.begin(115200); //This pipes to the serial monitor
	//Serial.println("Hallo World");

	Timer1.initialize(1000*samplerate); // in us
	Timer1.attachInterrupt(nsi_cnt_ISR);
	Timer1.stop();
	
}


void loop()
{   
  //Key entry, this is a bit tricky due to the fact, that the LEONARDO is speaking US keyboard
  //so we have to use a mixture of Keyboard.write and Keyboard.press
  
  //"ENTER" key via A0
  if (digitalRead(A0) == LOW)    {
    Keyboard.write(176);
    delay(500); //wait a bit
  }
   //"TAB" key via A1
  if (digitalRead(A1) == LOW)    {
    Keyboard.write(9);
    delay(500); //wait a bit
  }
 
  //"@" key via pin 2
    if (digitalRead(2) == LOW)    {
     Keyboard.press(KEY_RIGHT_ALT);
     Keyboard.press('q');
     Keyboard.releaseAll();
    delay(500); //wait a bit
  }

  //"(" key via pin 3 >>
  if (digitalRead(3) == LOW)    {
     Keyboard.press(KEY_RIGHT_SHIFT);
     Keyboard.press('8');
     Keyboard.releaseAll();
     delay(500);
  }

  //")" key via pin 4
  if (digitalRead(4) == LOW)    {
     Keyboard.press(KEY_LEFT_SHIFT);
     Keyboard.press('9');
     Keyboard.releaseAll();
    delay(500); //wait a bit
  }

  //"[" key via pin 5
  if (digitalRead(5) == LOW)    {
     Keyboard.press(KEY_RIGHT_ALT);
     Keyboard.press('8');
     Keyboard.releaseAll();
    delay(500); //wait a bit
  }

  //"]" key via pin 6
  if (digitalRead(6) == LOW)    {
     Keyboard.press(KEY_RIGHT_ALT);
     Keyboard.press('9');
     Keyboard.releaseAll();
    delay(500); //wait a bit
  }

  //"{" key via pin 7
  if (digitalRead(7) == LOW)    {
     Keyboard.press(KEY_RIGHT_ALT);
     Keyboard.press('7');
     Keyboard.releaseAll();
     delay(500); //wait a bit
  }

  //"}" key via pin 8
  if (digitalRead(8) == LOW)    {
     Keyboard.press(KEY_LEFT_ALT); //I have to use this combination, because KEY_RIGHT_ALT is not working for "}"
     Keyboard.press(KEY_LEFT_CTRL);
     Keyboard.press('0');
     Keyboard.releaseAll();
     delay(500); //wait a bit
  }

  //"\" key via pin 11
  if (digitalRead(11) == LOW)    {   
     Keyboard.write(92); // printing a "\" is only working by using the ASCII code
     delay(500); //wait a bit
  }

  //"<" key via pin 12
  if (digitalRead(12) == LOW)    {
     Keyboard.press('<');
     Keyboard.releaseAll();
     delay(500); //wait a bit
  }

    //">" key via pin 13
  if (digitalRead(13) == LOW)    {
     Keyboard.press(KEY_RIGHT_SHIFT);
     Keyboard.press('<');
     Keyboard.releaseAll();
    delay(500); //wait a bit
  }
  
  
  //Dial
	if (!(digitalRead(NSApin))) {

		if (iwvcnt > iwv_old) {
			iwv_old = iwvcnt;
			//Serial.print(iwv_old);
			if (iwv_old > 1) {
				delay(5); //seems necessary , otherwise overrun the USB??
				//delete the previous digit, this results to the count up effect
				Keyboard.write(KEY_BACKSPACE);
				delay(5);
			};
			Keyboard.write((iwv_old%10) + '0');
		};

		if (nsa_low_cnt > 10) {
			//we have long enough a low signal
			if (!t1_started) {
				Timer1.restart(); //we start to count
				t1_started = true;
				//Serial.println("timer started");
			};
		} else nsa_low_cnt++;
		
		} else {
		nsa_low_cnt =0; // we are high -> reset
		if (nsa_high_cnt > 10) { //debounce
			if (t1_started) { //disc rotated to end
				Timer1.stop();
				//Serial.println(" timer stopped");
				t1_started = false;
				iwvcnt = 0;
				iwv_old = 0;
			}
		
		} else nsa_high_cnt++;
	}
}
