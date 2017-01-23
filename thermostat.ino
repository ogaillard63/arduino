#include <Wire.h>
#include <TinyOLED.h>
#include <OneWire.h>
#include <RCSwitch.h>
#include <Button.h>
#include "data.h"

#define DEBUGMODE        1
// OLED I2C bus address
#define OLED_address  0x3c
#define BUTTON1_PIN     12
#define DS18B20_PIN     2
#define LED_PIN         13
#define REMOTE_TX_PIN   7

// Déclaration de l'objet ds
OneWire ds(DS18B20_PIN); // on pin DS18B20_PIN (a 4.7K resistor is necessary)

TinyOLED tinyOLED(0x3C); // SPI Address
RCSwitch mySwitch = RCSwitch();

const int DS18B20_ID=0x28;

// GND -----/ button ------ pin 12
Button button1 = Button(BUTTON1_PIN,BUTTON_PULLUP_INTERNAL);
boolean longPush = false;
boolean refresh = true; // refresh display
char buf [4];

float temperature;
float last_temperature = 0.0f;

// debug
int index = 0;
int sampleTemp[] = { 15,16,17,18,19,20,21,22,23,24,25,24,23,22,21,20,19,18,17,16};      

boolean heatingOn = false;
boolean setOn = false;
boolean isHold = false;

float seuil = 18;
float hysteresis = 1;

float tempMin = 14;
float tempMax = 25;

void setup() {
  if (DEBUGMODE) {
    Serial.begin(9600);
    Serial.println("Initialisation du programme");
  }
  // Init button
  button1.releaseHandler(handleButtonReleaseEvents);
  button1.holdHandler(handleButtonHoldEvents,2000);
 
  mySwitch.enableTransmit(REMOTE_TX_PIN);
  mySwitch.setPulseLength(450); //set pulse length
  mySwitch.setProtocol(1, 450); // set protocol
  mySwitch.setRepeatTransmit(10); // transmission repetitions
  
  // test SWITCH
 /*
   mySwitch.send(1381719, 24); // turnOn
   delay(2000);
   mySwitch.send(1381716, 24); // turnOff
   delay(2000);
*/
  // Initialize I2C and OLED Display
  Wire.begin();
  tinyOLED.init();  //init OLED display
  initScreen();
}

void loop() {  
 button1.isPressed();

 if (!setOn) {
   temperature = getTemperatureDS18b20(); // On lance la fonction d'acquisition de T° 
   if (temperature < (seuil -(hysteresis/2))) turnOn();
   if (temperature > (seuil + (hysteresis/2))) turnOff();
   displayInfos();
 }
 
 if (DEBUGMODE) {
    Serial.print("Temperature : "); 
    Serial.println(temperature);
    }
  }

void showTemperature(float t) {
  int partent = (int) t;
  int partdec = (t - partent)*10;

  sprintf (buf, "%i,%i", partent, partdec);
  tinyOLED.drawBigNums(0,2, buf);
}

void showLocation() {
  tinyOLED.drawStringXY(0,2, "45.6541358"); 
  tinyOLED.drawStringXY(0,4, "03.2458689"); 
}

void turnOn() {
    mySwitch.send(1381719, 24);
    delay(500);
    digitalWrite(LED_PIN, HIGH);
    heatingOn = true;
    if (DEBUGMODE) Serial.println("Heating ON !");
}

void turnOff() {
    digitalWrite(LED_PIN, LOW);
    delay(500);
    mySwitch.send(1381716, 24);
    heatingOn = false;    
    if (DEBUGMODE) Serial.println("Heating OFF !");
}

/* -------- Gestion de l'affichage -------- */
void displayInfos() {
  initScreen();   // clears the screen and buffer
  showTemperature(temperature);
  if (setOn) {
    tinyOLED.drawStringXY(1, 7, "SETTING");
  }
  else {
    if (heatingOn) tinyOLED.drawStringXY(1, 7, "ON           ");
    else tinyOLED.drawStringXY(1, 7, "OFF          ");
  }

  sprintf (buf, "%i", (int)(seuil*100));
  tinyOLED.drawStringXY(10, 7, buf);
}

void initScreen() {
  tinyOLED.drawStringXY(0,0, " THERMOSTAT"); 
  showBatterylevel(readVcc());
 /* for(byte row=2; row<8; row++) {	
    tinyOLED.setXY(0,row);    
    for(byte col=0; col<128; col++) tinyOLED.sendData(0);
  }*/
}

/* ---------- Gestion du boutons ---------- */

// Bouton relaché
void handleButtonReleaseEvents(Button &btn) {
  if (!isHold) {
    if (setOn) {
        flashLed();
        seuil+=0.5f;
        if (seuil > tempMax) seuil = tempMin;
        displayInfos();
        }
    }
    isHold = false;
  }
  
// Appui prolongé sur le bouton
void handleButtonHoldEvents(Button &btn)  {
  isHold = true;
  if (setOn == true) {
    setOn = false;
    digitalWrite(LED_PIN, LOW);
    displayInfos();
  }
  
  else {
    setOn = true;
    digitalWrite(LED_PIN, HIGH);
    displayInfos();
  }
 }
 
/* --------------------------------------------------------- */
void flashLed(){
    digitalWrite(LED_PIN, LOW);   // turn the LED on (HIGH is the voltage level)
    delay(100);               // wait for a second
    digitalWrite(LED_PIN, HIGH);    // turn the LED off by making the voltage LOW
    delay(100);
} 
/* --------------- Acquisition de la température ----------------------------------- */
float _getTemperatureDS18b20(){
  if (index++ > 18) index = 0;
  delay(850); 
  return (float)sampleTemp[index];
}

float getTemperatureDS18b20(){
  byte i;
  byte data[12];
  byte addr[8];
  float temp =0.0;
  
  //Il n'y a qu'un seul capteur, donc on charge l'unique adresse.
  ds.search(addr);
  
  // Cette fonction sert à surveiller si la transmission s'est bien passée
  if (OneWire::crc8( addr, 7) != addr[7]) {
  return false;
  }
  
  // On vérifie que l'élément trouvé est bien un DS18B20
  if (addr[0] != DS18B20_ID) {
  return false;
  }
  
  // Demander au capteur de mémoriser la température et lui laisser 850ms pour le faire (voir datasheet)
  ds.reset();
  ds.select(addr);
  ds.write(0x44);
  delay(850);
  // Demander au capteur de nous envoyer la température mémorisé
  ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  
  // Le MOT reçu du capteur fait 9 octets, on les charge donc un par un dans le tableau data[]
  for ( i = 0; i < 9; i++) {
  data[i] = ds.read();
  }
  // Puis on converti la température (*0.0625 car la température est stockée sur 12 bits)
  temp = ( (data[1] << 8) + data[0] )*0.0625;
  
  return temp;
}


/* ---- */
// Show battery level icon
void showBatterylevel(long vcc) {
  byte level = 0;
  if (vcc > 3600) level=4; 
  else if (vcc > 3400) level=3; 
  else if (vcc > 3200) level=2; 
  else if (vcc > 3000) level=1; 
  tinyOLED.drawBitmap(14, 0, battery[level], 16, 8); 
}
// Read VCC
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}
