/**
 * Project :  Dual LED matrix 8x8 clock
 * Author : Olivier Gaillard
 * LastMod : 14/03/2014
 **/

#define DEBUGMODE        1

#include <Button.h>
#include <LedControl.h> 
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

RTC_DS1307 RTC;

#define BUTTON1_PIN     4
#define ONE_WIRE_BUS    5

byte splash[6*8] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,      
  0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 
  0x00, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x00,
  0x00, 0x00, 0x3C, 0x24, 0x24, 0x3C, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

byte car[12*4] = {
  0x00, 0xFE, 0x82, 0xFE, // 0
  0x00, 0x84, 0xFE, 0x80, // 1
  0x00, 0xF2, 0x92, 0x9E, // 2
  0x00, 0x92, 0x92, 0xFE, // 3
  0x00, 0x1E, 0x10, 0xFE, // 4
  0x00, 0x9E, 0x92, 0xF2, // 5
  0x00, 0xFE, 0x92, 0xF2, // 6
  0x00, 0x02, 0x02, 0xFE, // 7
  0x00, 0xFE, 0x92, 0xFE, // 8
  0x00, 0x9E, 0x92, 0xFE, // 9
  0x00, 0x1C, 0x14, 0x1C, // ° 
  0x00, 0xFE, 0x82, 0x82  // C 
};

int etat = 0; // 1 -> set hours | 2 -> set minutes

DateTime now;
int offsetSec = 0;
const int devCount = 2;
int brightness = 0;
int device = 0;
boolean isHidden = false;
boolean longPush = false;
int value = 0;
int loopTime = 10;
int actualTemp = 0;
int sec; 

// DIN, CLK, LOAD (CS), number of matrix
LedControl lc=LedControl(12,11,10,devCount); 
Button button1 = Button(BUTTON1_PIN,BUTTON_PULLDOWN);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void handleButtonReleaseEvents(Button &btn) {
  if (DEBUGMODE) Serial.println("releas");
  if (etat != 0 && !longPush) {
      value = 1;
    }
  longPush = false;
}

void handleButtonHoldEvents(Button &btn) {
  if (DEBUGMODE) Serial.println("hold");
  longPush = true;
  loopTime = 100;
  if (++etat > 2) {
      loopTime = 10;
      etat = 0;
    }
}

void setup(){
  if (DEBUGMODE) Serial.begin(57600);
  button1.releaseHandler(handleButtonReleaseEvents);
  button1.holdHandler(handleButtonHoldEvents,2000);

  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  now = RTC.now();
  sensors.begin();
  for(int d=0;d<devCount;d++) {
    lc.shutdown(d,false);
    lc.setIntensity(d,brightness);
    lc.clearDisplay(d);
  }
  showSplash();
}

void loop(){
  // Lecture des boutons
  button1.isPressed();

  if (etat == 1 ) { // reglage des heures
    offsetSec = 3600*value;
  }
  if (etat == 2 ) { // reglage des minutes
    offsetSec = 60*value;
  }

  if (offsetSec != 0 ) {
    DateTime newTime (now.unixtime() + offsetSec); 
    RTC.adjust(newTime);
    offsetSec = 0;
    value = 0;
  }

  if (RTC.isrunning()) {
    now = RTC.now();
  }

  // Affiche la temperature toutes les 20 sec pendant 5 sec
  sec = now.second();
  if (etat == 0 && ((sec > 0 && sec < 6) || (sec > 20 && sec < 26) || (sec > 40 && sec < 46))) {
    sensors.requestTemperatures(); // Send the command to get temperatures
    actualTemp = sensors.getTempCByIndex(0);  
    showTemp();
  }
 else showClock();

  delay(loopTime);
} 

// ----- Functions -----
void showClock() {
  if (etat != 2 || isHidden) { // minutes
    showCar(now.minute()/10, 0, 0);
    showCar(now.minute() -((now.minute()/10)*10), 0, 4);
  }
  else {
    lc.clearDisplay(0);
  }
  if (etat != 1 || isHidden) { // heures
    showCar(now.hour()/10, 1, 0);
    showCar(now.hour() -((now.hour()/10)*10), 1, 4);
  }
  else {
    lc.clearDisplay(1);
  }
  isHidden = !isHidden;
}

void showTemp() {
  showCar(10, 0, 0);
  showCar(11, 0, 4);

  showCar(actualTemp/10, 1, 0);
  showCar(actualTemp -((actualTemp/10)*10), 1, 4);
}

void showCar(int val, int device, int offset) {
  for(int i=0;i<4;i++) {
    lc.setRow(device,i+offset,car[i+val*4]);
  }  
}
void showSplash() {
  for(int f=0;f<6;f++) {
    for(int i=0;i<8;i++) {
      lc.setRow(0,i,splash[i+f*8]);
      lc.setRow(1,i,splash[i+f*8]);
    }
    delay(200);  
  }
}