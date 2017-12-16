#include "OneButton.h"
#include "LowPower.h"
#include <EEPROM.h>

const int led1          = 3;
const int led2          = 5;
const int push_button   = 2;
int lightIntensity      = 50;
int savedIntensity;
int addr                = 0;
long int count          = 0;
long int autoOff        = 0;

enum {LIGHTON, LIGHTOFF, LIGHTMAX, SETTING_ON, SETTING, SETTING_OFF, INCR};
uint8_t STATE;

// Setup a new OneButton on pin 2.
OneButton button(push_button, true);

void setup() {
  button.attachClick(click);
  button.attachDoubleClick(doubleclick);
  button.attachLongPressStart(longPressStart);
  button.attachLongPressStop(longPressStop);
  button.attachDuringLongPress(longPress);
  button.setPressTicks(3000); // 3s long press
  // lecture de la valeur sauvegardée
  savedIntensity = EEPROM.read(addr);
  lightIntensity = max(savedIntensity, 10);
  STATE = LIGHTOFF;

}

void loop() {
  button.tick();

  switch (STATE) {

    case LIGHTON:
      turnLightOn();
      autoOff++;
      count = 0;
      break;

    case LIGHTOFF:
      turnLightOff();
      autoOff = 0; // reset autoOff
     if (count++ > 1000) { // Delay before powerDown
        //flashBothLight(); // debug
        attachInterrupt(digitalPinToInterrupt(push_button), wakeUp, CHANGE);
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        detachInterrupt(digitalPinToInterrupt(push_button));
        count = 0;
     }
      break;

    case LIGHTMAX:
      autoOff++;
      count = 0;
      setLight(255);
      break;

    case SETTING_ON: // entering Setting Mode
      autoOff = 0; // reset autoOff
      turnLightOff();
      flashLight(led1);
      STATE = SETTING;
      break;

    case SETTING_OFF: // leaving Setting Mode
      turnLightOff();
      flashLight(led2);
      if (savedIntensity != lightIntensity) EEPROM.write(addr, lightIntensity);
      STATE = LIGHTOFF;
      break;

    case SETTING: // Setting Mode
      turnLightOn();
      break;

    case INCR: //increment the counter
      lightIntensity += 10;
      if (lightIntensity > 250) lightIntensity = 0;
      STATE = SETTING;
      break;
  }
  // Turn off after 120sec
  if (autoOff++ > 45000) STATE = LIGHTOFF; // 375 = 1s
  delay(10);
}

void wakeUp() {
}

void click() {
  if (STATE == SETTING ) STATE = INCR;
  else if (STATE == LIGHTOFF ) STATE = LIGHTON;
  else if (STATE == LIGHTON ) STATE = LIGHTOFF;
  else if  (STATE == LIGHTMAX ) STATE = LIGHTOFF;
}

void doubleclick() {
  if (STATE == LIGHTMAX ) STATE = LIGHTOFF;
  else if (STATE != SETTING ) STATE = LIGHTMAX;
}


void longPressStart() {
}


void longPress() {
  if (STATE == SETTING ) STATE = SETTING_OFF;
  else STATE = SETTING_ON;
}


void longPressStop() {

}

void turnLightOn() {
  setLight(lightIntensity);
}

void turnLightOff() {
  setLight(0);
}

void setLight(int intensity) {
  analogWrite(led1, intensity);
  analogWrite(led2, intensity);
}

void flashLight(int led) {
  turnLightOff();
  for (int i = 0; i <= 5; i++) {
    analogWrite(led, 150);
    delay(100);
    analogWrite(led, 0);
    delay(100);
  }
}

void flashBothLight() {
  turnLightOff();
  for (int i = 0; i <= 10; i++) {
    analogWrite(led1, 150);
    analogWrite(led2, 150);
    delay(50);
    analogWrite(led1, 0);
    analogWrite(led2, 0);
    delay(50);
  }
}

