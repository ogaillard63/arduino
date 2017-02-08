#include <Wire.h>
#include <TinyOLED.h>
#include <OneWire.h>
#include <RCSwitch.h>
#include <Button.h>
#include <DallasTemperature.h>
#include <avr/pgmspace.h>

// Generated with GLCD Tools (output Microchip)
// battery level 16x8
uint8_t const battery[][16] PROGMEM = {
    {0x1c, 0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f},
    {0x1c, 0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x5d, 0x5d, 0x41, 0x7f},
    {0x1c, 0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x5d, 0x5d, 0x41, 0x5d, 0x5d, 0x41, 0x7f},
    {0x1c, 0x7f, 0x41, 0x41, 0x41, 0x41, 0x5d, 0x5d, 0x41, 0x5d, 0x5d, 0x41, 0x5d, 0x5d, 0x41, 0x7f},
    {0x1c, 0x7f, 0x41, 0x5d, 0x5d, 0x41, 0x5d, 0x5d, 0x41, 0x5d, 0x5d, 0x41, 0x5d, 0x5d, 0x41, 0x7f}};

#define DEBUGMODE 0
#define OLED_address 0x3c // OLED I2C bus address
#define BUTTON1_PIN 12
#define DS18B20_PIN 2
#define TEMPERATURE_PRECISION 10 // Lower resolution
#define REMOTE_TX_PIN 7
#define SWITCH_ON_CODE 1381719
#define SWITCH_OFF_CODE 1381716

// Déclaration de l'objet ds
OneWire oneWire(DS18B20_PIN); // on pin DS18B20_PIN (a 4.7K resistor is necessary)
DallasTemperature sensors(&oneWire);
TinyOLED tinyOLED(0x3C); // SPI Address
RCSwitch mySwitch = RCSwitch();

// GND -----/ button ------ pin 12
Button button1 = Button(BUTTON1_PIN, BUTTON_PULLUP_INTERNAL);
char buf[4];

float temperature;
float last_temperature = 0.0f;

boolean heatingOn = false;
boolean setOn = false;
boolean isHold = false;

float threshold = 18;
float hysteresis = 1;
float tempMin = 14;
float tempMax = 25;
DeviceAddress tempDeviceAddress; // sensor device address

void setup() {
  if (DEBUGMODE) {
    Serial.begin(9600);
    Serial.println("RF Thermostat init ...");
  }
  // Init button
  button1.releaseHandler(handleButtonReleaseEvents);
  button1.holdHandler(handleButtonHoldEvents, 2000);

  mySwitch.enableTransmit(REMOTE_TX_PIN);
  mySwitch.setPulseLength(450);   //set pulse length
  mySwitch.setProtocol(1, 450);   // set protocol
  mySwitch.setRepeatTransmit(10); // transmission repetitions

  // Activate sensors
  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

  // Initialize I2C and OLED Display
  Wire.begin();
  tinyOLED.init(); //init OLED display
  displayHeader();
}

void loop() {
  button1.isPressed();
  if (!setOn) {
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    if (last_temperature != temperature) {
      last_temperature = temperature;
      if (temperature < (threshold - (hysteresis / 2))) turnOn();
      if (temperature > (threshold + (hysteresis / 2))) turnOff();
      displayInfos();
      debug("Temperature : ", temperature);
    }
  }
  
}


// Turn ON Remote
void turnOn() {
  mySwitch.send(SWITCH_ON_CODE, 24);
  delay(500);
  heatingOn = true;
  if (DEBUGMODE)
    Serial.println("Heating ON !");
}
// Turn OFF Remote
void turnOff() {
  delay(500);
  mySwitch.send(SWITCH_OFF_CODE, 24);
  heatingOn = false;
  if (DEBUGMODE)
    Serial.println("Heating OFF !");
}
// Display screen infos
void displayInfos() {
  displayHeader(); 
  showTempData(temperature, 1);
  if (setOn) {
    tinyOLED.drawStringXY(7, 7, "SET");
  }
  else {
    if (heatingOn)
      tinyOLED.drawStringXY(1, 7, "ON       ");
    else
      tinyOLED.drawStringXY(1, 7, "OFF      ");
  }
  showTempData(threshold, 2);
}
// Display screen header
void displayHeader() {
  tinyOLED.drawStringXY(0, 0, " THERMOSTAT");
  showBatterylevel(readVcc());
}
// Display temperature on screen
void showTempData(float t, int pos) {
  float dec = (t-(int)t) * 10;
  sprintf(buf, "%d,%d", (int)t, (int)dec);
  if (pos == 1) tinyOLED.drawBigNums(2, 2, buf);
  if (pos == 2) tinyOLED.drawStringXY(11, 7, buf);
  
}
// Button is Release
void handleButtonReleaseEvents(Button &btn) {
  if (!isHold) {
    if (setOn) {
      threshold += 0.5f;
      if (threshold > tempMax)
        threshold = tempMin;
      displayInfos();
    }
  }
  isHold = false;
}
// Button is hold
void handleButtonHoldEvents(Button &btn) {
 isHold = true;
 setOn = !setOn ;
 displayInfos();
}
// Show battery level icon
void showBatterylevel(long vcc) {
  byte level = 0;
  if (vcc > 3600)
    level = 4;
  else if (vcc > 3400)
    level = 3;
  else if (vcc > 3200)
    level = 2;
  else if (vcc > 3000)
    level = 1;
  tinyOLED.drawBitmap(14, 0, battery[level], 16, 8);
}
// Read VCC
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);            // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}
// Display debug infos
void debug(String str, float value) {
 if (DEBUGMODE) Serial.print(str);
 if (DEBUGMODE) Serial.println(value);
} 