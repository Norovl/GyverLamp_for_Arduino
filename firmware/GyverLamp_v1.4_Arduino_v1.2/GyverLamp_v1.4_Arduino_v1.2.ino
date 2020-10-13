/*
  Скетч к проекту "Многофункциональный RGB светильник"
  Страница проекта (схемы, описания): https://alexgyver.ru/GyverLamp/
  Исходники на GitHub: https://github.com/AlexGyver/GyverLamp/
  Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver, AlexGyver Technologies, 2019
  https://AlexGyver.ru/
*/

/*
  Версия 1.4:
  - Исправлен баг при смене режимов
  - Исправлены тормоза в режиме точки доступа
*/

//// Ссылка для менеджера плат:
//// http://arduino.esp8266.com/stable/package_esp8266com_index.json

// ============= НАСТРОЙКИ =============

//// -------- РАССВЕТ -------
//#define DAWN_BRIGHT 200       // макс. яркость рассвета
//#define DAWN_TIMEOUT 1        // сколько рассвет светит после времени будильника, минут

// ---------- МАТРИЦА ---------
#define BRIGHTNESS 40         // стандартная маскимальная яркость (0-255)
#define CURRENT_LIMIT 2000    // лимит по току в миллиамперах, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит

#define WIDTH 16              // ширина матрицы
#define HEIGHT 16             // высота матрицы

#define COLOR_ORDER GRB       // порядок цветов на ленте. Если цвет отображается некорректно - меняйте. Начать можно с RGB

#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 0    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 0     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
// при неправильной настройке матрицы вы получите предупреждение "Wrong matrix parameters! Set to default"
// шпаргалка по настройке матрицы здесь! https://alexgyver.ru/matrix_guide/

#define numHold_Time 1500     // время отображения индикатора уровня яркости/скорости/масштаба

// ============= ДЛЯ РАЗРАБОТЧИКОВ =============
#define LED_PIN 2             // пин ленты
#define BTN_PIN 4
#define MODE_AMOUNT 18
#define NUM_LEDS WIDTH * HEIGHT
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)

// ---------------- БИБЛИОТЕКИ -----------------
//#include "timerMinim.h"
#include <EEPROM.h>
#include <FastLED.h>
#include <GyverButton.h>

// ------------------- ТИПЫ --------------------
CRGB leds[NUM_LEDS];
//timerMinim timeTimer(3000);
GButton touch(BTN_PIN, LOW_PULL, NORM_OPEN);  //сенсорная кнопка
//GButton touch(BTN_PIN, HIGH_PULL, NORM_OPEN);  //механическая кнопка

// ----------------- ПЕРЕМЕННЫЕ ------------------

//String inputBuffer;
static const byte maxDim = max(WIDTH, HEIGHT);
struct {
  byte brightness = 50;
  byte speed = 30;
  byte scale = 10;
} modes[MODE_AMOUNT];

//struct {
//  boolean state = false;
//  int time = 0;
//} alarm[7];

//byte dawnOffsets[] = {5, 10, 15, 20, 25, 30, 40, 50, 60};
//byte dawnMode;
//boolean dawnFlag = false;
//long thisTime;
//boolean manualOff = false;

int8_t currentMode = 17;
boolean loadingFlag = true;
boolean ONflag = true;
byte numHold;
unsigned long numHold_Timer = 0;
//uint32_t eepromTimer;
//boolean settChanged = false;
// Конфетти, Огонь, Радуга верт., Радуга гориз., Смена цвета,
// Безумие 3D, Облака 3D, Лава 3D, Плазма 3D, Радуга 3D,
// Павлин 3D, Зебра 3D, Лес 3D, Океан 3D,
// colorRoutine, snowRoutine, полосы "Матрица"

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

  Serial.begin(9600);
  Serial.println();

  if (EEPROM.read(0) == 102) {                    // если было сохранение настроек, то восстанавливаем их (с)НР
    currentMode = EEPROM.read(1);
    for (byte x = 0; x < MODE_AMOUNT; x++) {
      modes[x].brightness = EEPROM.read(x * 3 + 11); // (2-10 байт - резерв)
      modes[x].speed = EEPROM.read(x * 3 + 12);
      modes[x].scale = EEPROM.read(x * 3 + 13);
    }

  }
}

void loop() {
  effectsTick();
  //timeTick();
  buttonTick();
  //yield();
}
