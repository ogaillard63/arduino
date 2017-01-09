/**
 * Project : 4 matrix 8x8 clock
 * Author : Olivier Gaillard
 * LastMod : 14/03/2014
*/

#include <LedControl.h> 
#include <FlexiTimer2.h>
#include <Time.h>
#include <Wire.h>
#include <RTClib.h>
#include <Button.h>
RTC_DS1307 RTC;

#define BUTTON1_PIN   4
#define DEBUG_MODE    0

byte Font8x6[12*8] = {
  0x78,0xfc,0xcc,0xcc,0xcc,0xcc,0xfc,0x78, // 0
  0x70,0x78,0x78,0x70,0x70,0x70,0x70,0x70, // 1
  0x78,0xfc,0xec,0xec,0x70,0x38,0xfc,0xfc, // 2
  0x78,0xfc,0xe4,0xf0,0x70,0xe4,0xfc,0x78, // 3
  0x60,0x70,0x78,0x6c,0xec,0xfc,0x70,0x70, // 4
  0xfc,0xfc,0x0c,0x7c,0xf8,0xc0,0xfc,0x7c, // 5
  0x78,0x7c,0xc,0x7c,0xfc,0xcc,0xfc,0x78, // 6
  0xfc,0xfc,0xe0,0x70,0x70,0x38,0x38,0x38, // 7
  0x78,0xfc,0xcc,0xfc,0x78,0xcc,0xfc,0x78, // 8
  0x78,0xfc,0xcc,0xfc,0xf8,0xc0,0xf8,0x78, // 9
  0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00, // dot left
  0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00  // dot right
};
const int nbDevices = 4;
int brightness = 0;
int etat = 0; // 1 -> set hours | 2 -> set minutes
int value = 0;
boolean isHidden = false;
boolean longPush = false;
boolean showDot = true;
boolean displayOn = false;

DateTime now;
int lastSec = 0;
int offsetSec = 0;
int loopTime = 10;

// inputs: DIN pin, CLK pin, LOAD pin. number of chips
LedControl lc=LedControl(12,11,10,nbDevices); 
Button button1 = Button(BUTTON1_PIN,BUTTON_PULLUP);

void handleButtonReleaseEvents(Button &btn) {
  if (DEBUG_MODE == 1) Serial.println("ButtonRelease");
  if (etat != 0 && !longPush) {
    value = 1;
  }
  longPush = false;
}

void handleButtonHoldEvents(Button &btn) {
  if (DEBUG_MODE == 1) Serial.println("ButtonHold");
  longPush = true;
  loopTime = 100;
  if (++etat > 2) {
    loopTime = 10;
    etat = 0;
  }
}

void setup(){
  resetAll();
  if (DEBUG_MODE == 1) Serial.begin(57600);
  button1.releaseHandler(handleButtonReleaseEvents);
  button1.holdHandler(handleButtonHoldEvents,2000);
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  RTC.begin();

  if (! RTC.isrunning()) {
    if (DEBUG_MODE == 1) Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
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
    displayOn = true;
    //if (etat == 0) loopTime = 1000;
  }

  if (displayOn) showClock();
  else showWaitScreen();
  delay(loopTime);
}

// Affiche les 4 digits de l'horloge
void showClock() {
  if (lastSec != now.second()) showDot = !showDot;

  if (etat != 1 || isHidden) { // heures
    showNumber(3, now.hour()/10, 1, 0);
    showNumber(2, now.hour() -((now.hour()/10)*10), 2, (showDot) ? 2 : 0);
  }
  else {
    lc.clearDisplay(3);
    lc.clearDisplay(2); // Affiche les points
  }
  if (etat != 2 || isHidden) { // minutes
    showNumber(1, now.minute()/10, 0, (showDot) ? 1 : 0);
    showNumber(0, now.minute() -((now.minute()/10)*10), 1, 0);
  }
  else {
    lc.clearDisplay(1); // Affiche les points
    lc.clearDisplay(0);
  }
  lastSec = now.second();
  isHidden   = !isHidden;
}

// Affiche un chiffre sur une matrice
void showNumber(int device, int number, int offset, int dot) {
  for (int k=0;k<8;k++) {
    byte bits = Font8x6[(number*8)+7-k];
    bits = bits>>offset;
    if (dot == 1 ) bits+= Font8x6[(11*8)+7-k];
    if (dot == 2 ) bits+= Font8x6[(10*8)+7-k];
    lc.setRow(device, k, bits);
  }
}
// Affiche une animation d'attente
void showWaitScreen() {
  for (int i=0;i<10;i++) {
    for (int d=0;d<4;d++) lc.setRow(d, 4, B01100110);
    delay(200);
    for (int d=0;d<4;d++) lc.setRow(d, 4, B10011001);
    delay(200);
  }
}
// Efface les matrices
void resetAll() {
  for(int device=0;device<nbDevices;device++) {
    lc.shutdown(device,false);
    lc.setIntensity(device,brightness);
    lc.clearDisplay(device);
  }
}