/*
  Скетч к проекту "Многофункциональный RGB светильник"
  Страница проекта (схемы, описания): https://alexgyver.ru/GyverLamp/
  Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver, AlexGyver Technologies, 2019(Портировал на Ардуино Norvol(+ эффекты, демо от stepko365) 
  https://AlexGyver.ru/
*/
// ---------------- БИБЛИОТЕКИ -----------------
#include <EEPROM.h>
#include <FastLED.h>
#include <GyverButton.h>
//-----------------            -----------------
#define FASTLED_USE_PROGMEM 1 // просим библиотеку FASTLED экономить память контроллера на свои палитры
#include "Constants.h"
// ----------------- ПЕРЕМЕННЫЕ ------------------
static const byte maxDim = max(WIDTH, HEIGHT);
struct { byte Brightness = 10; byte Speed = 30; byte Scale = 10; } modes[MODE_AMOUNT]; //настройки эффекта по умолчанию
int8_t currentMode = 10;
boolean loadingFlag = true;
boolean ONflag = true;
byte numHold;
unsigned long numHold_Timer = 0;
unsigned char matrixValue[8][16];


void setup() {
  // ЛЕНТА
  FastLED.addLeds<WS2812B, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)/*.setCorrection( TypicalLEDStrip )*/;
  FastLED.setBrightness(BRIGHTNESS);
  if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  FastLED.clear();
  FastLED.show();

  touch.setStepTimeout(100);
  touch.setClickTimeout(500);

  //Serial.begin(9600);
  //Serial.println();

  if (EEPROM.read(0) == 102) {                    // если было сохранение настроек, то восстанавливаем их (с)НР
    currentMode = EEPROM.read(1);
    for (byte x = 0; x < MODE_AMOUNT; x++) {
      modes[x].Brightness = EEPROM.read(x * 3 + 11); // (2-10 байт - резерв)
      modes[x].Speed = EEPROM.read(x * 3 + 12);
      modes[x].Scale = EEPROM.read(x * 3 + 13);
    }

  }
}

void loop() {
  effectsTick();
  buttonTick();
  demo();
}
