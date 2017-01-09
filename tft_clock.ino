/**
 * Flip-Clock
 * Desinged for Arduino ATMEGA328P / TFT 2.2" 240x320 display
 **/
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"
#include <Wire.h>
#include <RTClib.h>
#include <Button.h>

// These are the pins used for the UNO
#define TFT_SCLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_CS    5
#define TFT_DC    6
#define TFT_RST   4
#define TFT_BL_ON   {DDRD |= 0x80;PORTD |=  0x80;}

#define	WHITE            tft.Color565(255,255,255)
#define	BLACK            tft.Color565(0,0,0)
#define	RED              tft.Color565(255,58,47)
#define	GREEN            tft.Color565(58,255,47)
#define	BLUE             tft.Color565(13,143,209)
#define	YELLOW           tft.Color565(209,193,13)
#define	DARK_GRAY        tft.Color565(40,40,40)
#define	GRAY             tft.Color565(60,60,60)
#define	LIGHT_GRAY       tft.Color565(150,150,150)

#define BUTTON1_PIN        9
#define BACKLIGHT_PIN      3
#define HOLD_DURATION      1000
#define SMALL_FONT         1
#define MEDIUM_FONT        2
#define BIG_FONT           3

extern uint8_t BigFont[];
extern uint8_t MediumFont[];
extern uint8_t SmallFont[];
extern uint8_t Symbol[];

RTC_DS1307 RTC;
Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_RST);
Button button1 = Button(BUTTON1_PIN,BUTTON_PULLDOWN);
boolean longPush = false;
byte value, etat, lastEtat = 0;
boolean forceRefresh = false;
byte dimCycle[4] = {30,60,120};
byte dimValue = 1;
int digitPos[4] = {10,82,172,244};
String dayNames[7] = {
  "DIM","LUN", "MAR","MER","JEU","VEN","SAM"};
String monthNames[13] = {
  "","JAN","FEV","MAR","AVR","MAI","JUN","JUI","AOU","SEP","OCT","NOV","DEC"}; 
String settingsStates[7] = {
  "","Set clock hours (+)","Set clock minutes (+)","Set clock date (+)", "Set clock date (-)", "Set clock year (+)","Set clock year (-)"}; 

DateTime now;
byte lastHour, lastMinute, lastSecond, lastDay, lastTemp, temp = 0xFF;
int lastYear = 0;
boolean showDots = true;  
int top = 23;

void setup() {
  Wire.begin();
  RTC.begin();

  button1.releaseHandler(handleButtonReleaseEvents);
  button1.holdHandler(handleButtonHoldEvents,HOLD_DURATION);

  pinMode(BACKLIGHT_PIN, OUTPUT);   
  analogWrite(BACKLIGHT_PIN, dimCycle[dimValue]);
  tft.begin();

  tft.fillScreen(DARK_GRAY);
  tft.setRotation(1);
  //RTC.adjust(DateTime(__DATE__, __TIME__));
}

void loop(void) {

  button1.isPressed();
  now = RTC.now();

  // Display hours
  if (lastHour != now.hour() || forceRefresh) {
    showDigit(digitPos[0], top, now.hour()/10, (etat == 1)?true:false);
    showDigit(digitPos[1], top, now.hour()%10, (etat == 1)?true:false);
    lastHour = now.hour();
  }
  // Display minutes
  if (lastMinute != now.minute() || forceRefresh) {
    showDigit(digitPos[2], top, now.minute()/10, (etat == 2)?true:false);
    showDigit(digitPos[3], top, now.minute()%10, (etat == 2)?true:false);
    lastMinute = now.minute();
  } 

  // Display flashing dots  
  if (lastSecond != now.second()){
    lastSecond = now.second();
    drawDots(top); // 2 points
  }

  // Display temperature
    temp = round(getTemperature());
    if ((temp != lastTemp && temp >= 0 && etat == 0 ) || (etat == 0 && forceRefresh)) {
      showTemp(digitPos[0]+160, top+121, temp);
      lastTemp = temp;
    }

  // Display date
  if (lastDay != now.day() || lastYear != now.year() || forceRefresh) {
    showDate(digitPos[0], top+121, now.dayOfWeek(), now.day(), now.month(), now.year(), (etat > 2 && etat < 7)?true:false);
    lastDay = now.day();
    lastYear = now.year();
  }

  // Set time or date
  if (value > 0) {
    switch (etat) {

      case 1: // Set hours
        RTC.adjust(DateTime (now.year(), now.month(), now.day(), (now.hour() <23) ? now.hour()+1 : 0, now.minute(), 0));
        break;

      case 2: // Set minutes
        RTC.adjust(DateTime (now.year(), now.month(), now.day(), now.hour(), (now.minute() <59) ? now.minute()+1 : 0, 0));
        break;

      case 3: // Set date + 1day
       RTC.adjust(now.unixtime() + 86400);
        break;

      case 4: // Set date - 1day
       RTC.adjust(now.unixtime() - 86400);
        break;

      case 5: // Set date + year
        RTC.adjust(DateTime ((now.year() <2100) ? now.year()+1 : 2000, now.month(), now.day(), now.hour(), now.minute(), 0));
        break;

      case 6: // Set date - year
        RTC.adjust(DateTime ((now.year() >2000) ? now.year()-1 : 2000, now.month(), now.day(), now.hour(), now.minute(), 0));
        break;

    }
    value = 0;  
  }

  forceRefresh = false; // cancel forceRefresh
  if (lastEtat != etat ) {
   tft.fillScreen(DARK_GRAY);
   showDebug(settingsStates[etat]);
   forceRefresh = true; // force refresh
   lastEtat = etat;
  }
  // delay(10);
}

// ---------------------------------- Fonctions ---------------------------------- //
void drawDots(int y) {
  tft.fillRect(155, y+23, 10, 10, (showDots) ? DARK_GRAY : WHITE);
  tft.fillRect(155, y+73, 10, 10, (showDots) ? DARK_GRAY : WHITE);
  showDots = !showDots;
}

void showDigit(int x, int y, int number, boolean isSet) {
  drawFlap(x, y, 66, 102, 20, 2);
  showCar(x+9, y+12 , number, BIG_FONT, (isSet)?GREEN:WHITE, GRAY);
}

/* Affiche la température */
void showTemp(int x, int y, int temp) {
  // background temperature
  for(int i=0; i<120; i+=60) {
    drawFlap(x+i, y, 56, 78, 20, 2);
  }
  showCar(x+8, y+5 , temp/10, MEDIUM_FONT, RED, GRAY);
  showCar(x+68, y+5 , temp%10, MEDIUM_FONT, RED, GRAY);
  showImage(x+118, y+9, Symbol, 24, 26, RED, DARK_GRAY);
}

void showDate(int x, int y, int wd, byte d, byte m, int yr, boolean isSet) {
  // weekday
  drawFlap(x, y, 85, 38, 12, 1);
  showCar(x+7, y+5, getLetterPos(dayNames[wd][0]), SMALL_FONT, (isSet)?GREEN:YELLOW, GRAY);
  showCar(x+32, y+5, getLetterPos(dayNames[wd][1]), SMALL_FONT, (isSet)?GREEN:YELLOW, GRAY);
  showCar(x+57, y+5, getLetterPos(dayNames[wd][2]), SMALL_FONT, (isSet)?GREEN:YELLOW, GRAY);

  // day
  drawFlap(x, y+40, 55, 38, 12, 1);
  showCar(x+5, y+45, d/10, SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);
  showCar(x+27, y+45, d%10, SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);

  // month
  drawFlap(x+58, y+40, 85, 38, 12, 1);
  showCar(x+58+7, y+45, getLetterPos(monthNames[m][0]), SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);
  showCar(x+58+32, y+45, getLetterPos(monthNames[m][1]), SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);
  showCar(x+58+57, y+45, getLetterPos(monthNames[m][2]), SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);

  // year
  if (etat != 0) { // show only during clock settings
    drawFlap(x+58+85+5, y+40, 106, 38, 12, 1);
    showCar(x+58+85+7, y+45, (int)yr/1000, SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);
    showCar(x+58+85+32, y+45, ((int)yr/100)%10, SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);
    showCar(x+58+85+57, y+45, ((int)yr/10)%10, SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);
    showCar(x+58+85+82, y+45, yr%10, SMALL_FONT, (isSet)?GREEN:BLUE, GRAY);
  }
}

int getLetterPos(char car) {
  return car-65+10; // position in the font file
}

// Affiche un chiffre en x, y
void showCar(int x, int y, int number, int font_select, int color, int bgColor) {
  // Affiche les caractères en 2 moitiés
  // font_size :  1 = SmallFont, 2 = MediumFont, 3 = BigFont
  uint8_t *font;
  int fw, fh;
  if (font_select < 4 && font_select > 0) { 
    switch(font_select) {
    case 1: 
      font = SmallFont; fw = 24; fh = 26; 
      break;
    case 2: 
      font = MediumFont; fw = 40; fh = 66; 
      break;
    case 3: 
      font = BigFont; fw = 48; fh = 76; 
      break;
    }
    int hh = fh/2; // half height
    int cs = (fw/8) * fh; // caracter size in octets
    for(int n=0; n<2; n++) {
      delay(30);
      tft.setAddrWindow(x, y+(n*+(hh+2)), x+fw-1, y+hh-1 + (n*+(hh+2))); 
      for (int i=0; i<(cs/2); i++) {
        setRow(pgm_read_byte_near(font+i+number*cs+(n*+(cs/2))), color, bgColor); 
      }
    }
  }
}

void showImage(int x, int y, uint8_t *image, int w, int h, int color, int bgColor) {
  int cs = (w/8) * h; // size in octets
  tft.setAddrWindow(x, y, x+w-1, y+h-1); 
  for (int i=0; i<cs; i++) {
    setRow(pgm_read_byte_near(image+i), color, bgColor);
  }

}

void setRow(byte row, int color, int bgColor) {
  for(int i=7;i>-1;i--) {
    tft.pushColor((bitRead(row, i) == 1)? color : bgColor);
  }
}

void drawFlap(int x, int y, int w, int h, int r, int p) {
  int hr = r/2;
  tft.fillRect(x, y, w, h, BLACK);
  tft.fillRect(x+2, y+2, w-4, (h/2)-3, GRAY);
  tft.fillRect(x+2, y+((h+2)/2), w-4, (h/2)-3, GRAY);
  tft.fillRect(x+2, y+(h/2)-hr, p*2, r, BLACK);
  tft.fillRect(x+2, y+(h/2)-hr+2, p, r-4, GRAY);
  tft.fillRect(x+w-p*2-2, y+(h/2)-hr, p*2, r, BLACK);
  tft.fillRect(x+w-p-2, y+(h/2)-hr+2, p, r-4, GRAY);
}

/* retourne la température */
#define CLOCK_ADDRESS 0x68
float getTemperature() {
  // Checks the internal thermometer on the DS3231 and returns the 
  // temperature as a floating-point value.
  byte temp;
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();

  Wire.requestFrom(CLOCK_ADDRESS, 2);
  temp = Wire.read();	// Here's the MSB
  return float(temp) + 0.25*(Wire.read()>>6);
}

// Gestion du bouton relaché
void handleButtonReleaseEvents(Button &btn) {
  //showDebug("Release | etat = " + String(etat));
  if (!longPush) {
    if (etat != 0 ) { // Settings
      value = 1;
    } 
    else { // select backlight dim
      dimValue = (++dimValue>2) ? 0 : dimValue;
      analogWrite(BACKLIGHT_PIN, dimCycle[dimValue]);
    }
  }
  longPush = false;
}

// Gestion de l'appui prolongé sur le bouton
void handleButtonHoldEvents(Button &btn) {
  longPush = true;
  value = 0;
  if (++etat > 6) {
    etat = 0;
  }
}

void showDebug(String msg) {
  tft.fillRect(0, 0, 320, 20, DARK_GRAY);
  tft.setCursor(3, 3);
  tft.setTextColor(ILI9340_WHITE);  
  tft.setTextSize(2);
  tft.println(msg);
}