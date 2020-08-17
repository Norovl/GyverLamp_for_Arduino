// ================================= ЭФФЕКТЫ ====================================
uint8_t wrapX(int8_t x) {
  return (x + WIDTH) % WIDTH;
}
uint8_t wrapY(int8_t y) {
  return (y + HEIGHT) % HEIGHT;
}
uint8_t deltaHue, deltaHue2; // ещё пара таких же, когда нужно много
uint8_t step; // какой-нибудь счётчик кадров или постедовательностей операций
uint8_t pcnt;
uint8_t line[WIDTH];
uint8_t deltaValue; // просто повторно используемая переменная
#define NUM_LAYERSMAX 2
uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT];
uint8_t shiftHue[HEIGHT];
uint8_t shiftValue[HEIGHT];

// палитра для типа реалистичного водопада (если ползунок Масштаб выставить на 100)
extern const TProgmemRGBPalette16 WaterfallColors_p FL_PROGMEM = {0x000000, 0x060707, 0x101110, 0x151717, 0x1C1D22, 0x242A28, 0x363B3A, 0x313634, 0x505552, 0x6B6C70, 0x98A4A1, 0xC1C2C1, 0xCACECF, 0xCDDEDD, 0xDEDFE0, 0xB2BAB9};
// добавлено изменение текущей палитры (используется во многих эффектах ниже для бегунка Масштаб)
const TProgmemRGBPalette16 *palette_arr[] = {
  &PartyColors_p,
  &OceanColors_p,
  &LavaColors_p,
  &HeatColors_p,
  &WaterfallColors_p,
  &CloudColors_p,
  &ForestColors_p,
  &RainbowColors_p,
  &RainbowStripeColors_p
};
const TProgmemRGBPalette16 *curPalette = palette_arr[0];
void setCurrentPalette() {
  if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
  curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale / 100.0F * ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01F))];
}
CRGB _pulse_color;
void blurScreen(fract8 blur_amount, CRGB *LEDarray = leds)
{
  blur2d(LEDarray, WIDTH, HEIGHT, blur_amount);
}
void dimAll(uint8_t value) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(value); //fadeToBlackBy
  }
}
void drawPixelXYF(float x, float y, CRGB color)
{
  // extract the fractional parts and derive their inverses
  uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
  // calculate the intensities for each affected pixel
  #define WU_WEIGHT(a,b) ((uint8_t) (((a)*(b)+(a)+(b))>>8))
  uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
                   WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {
    int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
    CRGB clr = getPixColorXY(xn, yn);
    clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
    clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
    clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
    drawPixelXY(xn, yn, clr);
  }
}

void drawCircleF(float x0, float y0, float radius, CRGB color){
  float x = 0, y = radius, error = 0;
  float delta = 1 - 2 * radius;

  while (y >= 0) {
    drawPixelXYF(x0 + x, y0 + y, color);
    drawPixelXYF(x0 + x, y0 - y, color);
    drawPixelXYF(x0 - x, y0 + y, color);
    drawPixelXYF(x0 - x, y0 - y, color);
    error = 2 * (delta + y) - 1;
    if (delta < 0 && error <= 0) {
      ++x;
      delta += 2 * x + 1;
      continue;
    }
    error = 2 * (delta - x) - 1;
    if (delta > 0 && error > 0) {
      --y;
      delta += 1 - 2 * y;
      continue;
    }
    ++x;
    delta += 2 * (x - y);
    --y;
  }
}

// --------------------------------- конфетти ------------------------------------
#define FADE_OUT_SPEED        (70U)                         // скорость затухания
void sparklesRoutine()
{
  for (uint8_t i = 0; i < modes[currentMode].Scale; i++)
  {
    uint8_t x = random(0U, WIDTH);
    uint8_t y = random(0U, HEIGHT);
    if (getPixColorXY(x, y) == 0U)
    {
      leds[XY(x, y)] = CHSV(random(0U, 255U), 255U, 255U);
    }
  }
  fader(FADE_OUT_SPEED);
}
// функция плавного угасания цвета для всех пикселей
void fader(uint8_t step)
{
  for (uint8_t i = 0U; i < WIDTH; i++)
  {
    for (uint8_t j = 0U; j < HEIGHT; j++)
    {
      fadePixel(i, j, step);
    }
  }
}
void fadePixel(uint8_t i, uint8_t j, uint8_t step)          // новый фейдер
{
  int32_t pixelNum = XY(i, j);
  if (getPixColor(pixelNum) == 0U) return;

  if (leds[pixelNum].r >= 30U ||
      leds[pixelNum].g >= 30U ||
      leds[pixelNum].b >= 30U)
  {
    leds[pixelNum].fadeToBlackBy(step);
  }
  else
  {
    leds[pixelNum] = 0U;
  }
}
// -------------------------------------- огонь ---------------------------------------------
#define SPARKLES 0        // вылетающие угольки вкл выкл
//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[8][16] PROGMEM = {
  {0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  },
  {0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  },
  {0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  },
  {0  , 32 , 64 , 128, 128, 64 , 32 , 0  , 0  , 32 , 64 , 128, 128, 64 , 32 , 0  },
  {32 , 64 , 96 , 160, 160, 96 , 64 , 32 , 32 , 64 , 96 , 160, 160, 96 , 64 , 32 },
  {64 , 96 , 128, 192, 192, 128, 96 , 64 , 64 , 96 , 128, 192, 192, 128, 96 , 64 },
  {96 , 128, 160, 255, 255, 160, 128, 96 , 96 , 128, 160, 255, 255, 160, 128, 96 },
  {128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const unsigned char hueMask[8][16] PROGMEM = {
  {25, 22, 11, 1 , 1 , 11, 19, 25, 25, 22, 11, 1 , 1 , 11, 19, 25 },
  {25, 19, 8 , 1 , 1 , 8 , 13, 19, 25, 19, 8 , 1 , 1 , 8 , 13, 19 },
  {19, 16, 8 , 1 , 1 , 8 , 13, 16, 19, 16, 8 , 1 , 1 , 8 , 13, 16 },
  {13, 13, 5 , 1 , 1 , 5 , 11, 13, 13, 13, 5 , 1 , 1 , 5 , 11, 13 },
  {11, 11, 5 , 1 , 1 , 5 , 11, 11, 11, 11, 5 , 1 , 1 , 5 , 11, 11 },
  {8 , 5 , 1 , 0 , 0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 , 0 , 1 , 5 , 8  },
  {5 , 1 , 0 , 0 , 0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 , 0 , 0 , 1 , 5  },
  {1 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 1  }
};
void fireRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    //FastLED.clear();
    generateLine();
  }
  if (pcnt >= 100) {
    shiftUp();
    generateLine();
    pcnt = 0;
  }
  drawFrame(pcnt);
  pcnt += 25;
}

// Случайным образом генерирует следующую линию (matrix row)

void generateLine() {
  for (uint8_t x = 0; x < WIDTH; x++) {
    line[x] = random(127, 255);
  }
}

void shiftUp() {
  for (uint8_t y = HEIGHT - 1; y > 0; y--) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15) newX = x - 15;
      if (y > 7) continue;
      matrixValue[y][newX] = matrixValue[y - 1][newX];
    }
  }

  for (uint8_t x = 0; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x - 15;
    matrixValue[0][newX] = line[newX];
  }
}

// рисует кадр, интерполируя между 2 "ключевых кадров"
// параметр pcnt - процент интерполяции

void drawFrame(int pcnt) {
  int nextv;

  //each row interpolates with the one before it
  for (unsigned char y = HEIGHT - 1; y > 0; y--) {
    for (unsigned char x = 0; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15) newX = x - 15;
      if (y < 8) {
        nextv =
          (((100.0 - pcnt) * matrixValue[y][newX]
            + pcnt * matrixValue[y - 1][newX]) / 100.0)
          - pgm_read_byte(&(valueMask[y][newX]));

        CRGB color = CHSV(
                       modes[currentMode].Scale * 2.5 + pgm_read_byte(&(hueMask[y][newX])), // H
                       255, // S
                       (uint8_t)max(0, nextv) // V
                     );

        leds[getPixelNumber(x, y)] = color;
      } else if (y == 8 && SPARKLES) {
        if (random(0, 20) == 0 && getPixColorXY(x, y - 1) != 0) drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else drawPixelXY(x, y, 0);
      } else if (SPARKLES) {

        // старая версия для яркости
        if (getPixColorXY(x, y - 1) > 0)
          drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else drawPixelXY(x, y, 0);

      }
    }
  }

  //Перавя стрка интерполируется со следующей "next" линией
  for (unsigned char x = 0; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x - 15;
    CRGB color = CHSV(
                   modes[currentMode].Scale * 2.5 + pgm_read_byte(&(hueMask[0][newX])), // H
                   255,           // S
                   (uint8_t)(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0) // V
                 );
    leds[getPixelNumber(newX, 0)] = color;
  }
}
// ---------------------------------------- радуга ------------------------------------------
byte hue; byte hue2;
void rainbowVertical() {
  hue += 2;
  for (byte j = 0; j < HEIGHT; j++) {
    CHSV thisColor = CHSV((byte)(hue + j * modes[currentMode].Scale), 255, 255);
    for (byte i = 0; i < WIDTH; i++)
      drawPixelXY(i, j, thisColor);
  }
}
void rainbowHorizontal() {
  hue += 2;
  for (byte i = 0; i < WIDTH; i++) {
    CHSV thisColor = CHSV((byte)(hue + i * modes[currentMode].Scale), 255, 255);
    for (byte j = 0; j < HEIGHT; j++)
      drawPixelXY(i, j, thisColor);   //leds[getPixelNumber(i, j)] = thisColor;
  }
}
void rainbowDiagonalRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    FastLED.clear();
  }

  hue += 8;
  for (uint8_t i = 0U; i < WIDTH; i++)
  {
    for (uint8_t j = 0U; j < HEIGHT; j++)
    {
      float twirlFactor = 3.0F * (modes[currentMode].Scale / 100.0F);      // на сколько оборотов будет закручена матрица, [0..3]
      CRGB thisColor = CHSV((uint8_t)(hue + ((float)WIDTH / HEIGHT * i + j * twirlFactor) * ((float)255 / maxDim)), 255, 255);
      drawPixelXY(i, j, thisColor);
    }
  }
}
// ---------------------------------------- ЦВЕТА -----------------------------
void colorsRoutine() {
  hue += modes[currentMode].Scale;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, 255);
  }
}

// --------------------------------- ЦВЕТ ------------------------------------
void colorRoutine() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(modes[currentMode].Scale * 2.5, modes[currentMode].Speed * 2.5, 255);
  }
}

// ------------------------------ снегопад 2.0 --------------------------------
/*void snowRoutine() {
  // сдвигаем всё вниз
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }

  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    // а также не даём двум блокам по вертикали вместе быть
    if (getPixColorXY(x, HEIGHT - 2) == 0 && (random(0, modes[currentMode].Scale) == 0))
      drawPixelXY(x, HEIGHT - 1, 0xE0FFFF - 0x101010 * random(0, 4));
    else
      drawPixelXY(x, HEIGHT - 1, 0x000000);
  }
  }
*/
// ------------------------------ МАТРИЦА ------------------------------
/*void matrixRoutine() {
  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    uint32_t thisColor = getPixColorXY(x, HEIGHT - 1);
    if (thisColor == 0)
      drawPixelXY(x, HEIGHT - 1, 0x00FF00 * (random(0, modes[currentMode].Scale) == 0));
    else if (thisColor < 0x002000)
      drawPixelXY(x, HEIGHT - 1, 0);
    else
      drawPixelXY(x, HEIGHT - 1, thisColor - 0x002000);
  }

  // сдвигаем всё вниз
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }
  }
*/
// ------------------------------ БЕЛАЯ ЛАМПА ------------------------------
/*void whiteLamp() {
  for (byte y = 0; y < (HEIGHT / 2); y++) {
    CHSV color = CHSV(100, 1, constrain(modes[currentMode].Brightness - (long)modes[currentMode].Speed * modes[currentMode].Brightness / 255 * y / 2, 1, 255));
    for (byte x = 0; x < WIDTH; x++) {
      drawPixelXY(x, y + 8, color);
      drawPixelXY(x, 7 - y, color);
    }
  }
  }*/
//--------------------------Шторм,Метель-------------------------
#define e_sns_DENSE (32U) // плотность снега - меньше = плотнее
void stormRoutine2(bool isColored)
{
  // заполняем головами комет
  uint8_t Saturation = 0U;    // цвет хвостов
  uint8_t e_TAIL_STEP = 127U; // длина хвоста
  if (isColored)
    Saturation = modes[currentMode].Scale * 2.55;
  else
  {
    e_TAIL_STEP = 255U - modes[currentMode].Scale * 2.55;
  }
  for (int8_t x = 0U; x < WIDTH - 1U; x++) // fix error i != 0U
  {
    if (!random8(e_sns_DENSE) &&
        !getPixColorXY(wrapX(x), HEIGHT - 1U) &&
        !getPixColorXY(wrapX(x + 1U), HEIGHT - 1U) &&
        !getPixColorXY(wrapX(x - 1U), HEIGHT - 1U))
    {
      drawPixelXY(x, HEIGHT - 1U, CHSV(random8(), Saturation, random8(64U, 255U)));
    }
  }

  // сдвигаем по диагонали
  for (int8_t y = 0U; y < HEIGHT - 1U; y++)
  {
    for (int8_t x = 0; x < WIDTH; x++)
    {
      drawPixelXY(wrapX(x + 1U), y, getPixColorXY(x, y + 1U));
    }
  }

  // уменьшаем яркость верхней линии, формируем "хвосты"
  for (int8_t i = 0U; i < WIDTH; i++)
  {
    fadePixel(i, HEIGHT - 1U, e_TAIL_STEP);
  }
}
//------------------Синусоид3-----------------------
//Stefan Petrick
void SinusoidRoutine()
{
  const uint8_t semiHeightMajor =  HEIGHT / 2 + (HEIGHT % 2);
  const uint8_t semiWidthMajor =  WIDTH / 2  + (WIDTH % 2) ;
  float e_s3_Speed = 0.004 * modes[currentMode].Speed + 0.015; // Speed of the movement along the Lissajous curves
  float e_s3_size = 3 * (float)modes[currentMode].Scale / 100.0 + 2;  // amplitude of the curves

  float time_shift = float(millis() % (uint32_t)(30000 * (1.0 / ((float)modes[currentMode].Speed / 255))));

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      CRGB color;

      float cx = y + float(e_s3_size * (sinf (float(e_s3_Speed * 0.003 * time_shift)))) - semiHeightMajor;  // the 8 centers the middle on a 16x16
      float cy = x + float(e_s3_size * (cosf (float(e_s3_Speed * 0.0022 * time_shift)))) - semiWidthMajor;
      float v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.r = v;

      cx = x + float(e_s3_size * (sinf (e_s3_Speed * float(0.0021 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_Speed * float(0.002 * time_shift)))) - semiHeightMajor;
      v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.b = v;

      cx = x + float(e_s3_size * (sinf (e_s3_Speed * float(0.0041 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_Speed * float(0.0052 * time_shift)))) - semiHeightMajor;
      v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.g = v;
      drawPixelXY(x, y, color);
    }
  }
}
//-------------------------Метаболз-------------------
//Stefan Petrick
void MetaBallsRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }

  float Speed = modes[currentMode].Speed / 127.0;

  // get some 2 random moving points
  uint8_t x2 = inoise8(millis() * Speed, 25355, 685 ) / WIDTH;
  uint8_t y2 = inoise8(millis() * Speed, 355, 11685 ) / HEIGHT;

  uint8_t x3 = inoise8(millis() * Speed, 55355, 6685 ) / WIDTH;
  uint8_t y3 = inoise8(millis() * Speed, 25355, 22685 ) / HEIGHT;

  // and one Lissajou function
  uint8_t x1 = beatsin8(23 * Speed, 0, 15);
  uint8_t y1 = beatsin8(28 * Speed, 0, 15);

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {

      // calculate distances of the 3 points from actual pixel
      // and add them together with weightening
      uint8_t  dx =  abs(x - x1);
      uint8_t  dy =  abs(y - y1);
      uint8_t dist = 2 * sqrt((dx * dx) + (dy * dy));

      dx =  abs(x - x2);
      dy =  abs(y - y2);
      dist += sqrt((dx * dx) + (dy * dy));

      dx =  abs(x - x3);
      dy =  abs(y - y3);
      dist += sqrt((dx * dx) + (dy * dy));

      // inverse result
      //byte color = modes[currentMode].Speed * 10 / dist;
      byte color = 1000U / dist;

      // map color between thresholds
      if (color > 0 and color < 60) {
        if (modes[currentMode].Scale == 100U)
          drawPixelXY(x, y, CHSV(color * 9, 255, 255));// это оригинальный цвет эффекта
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, color * 9));
      } else {
        if (modes[currentMode].Scale == 100U)
          drawPixelXY(x, y, CHSV(0, 255, 255)); // в оригинале центральный глаз почему-то крвсный
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, 0U));
      }
      // show the 3 points, too
      drawPixelXY(x1, y1, CRGB(255, 255, 255));
      drawPixelXY(x2, y2, CRGB(255, 255, 255));
      drawPixelXY(x3, y3, CRGB(255, 255, 255));
    }
  }
}

// ============= ЭФФЕКТ ВОЛНЫ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternWave.h
// Адаптация от (c) SottNick

byte waveThetaUpdate = 0;
byte waveThetaUpdateFrequency = 0;
byte waveTheta = 0;

byte hueUpdate = 0;
byte hueUpdateFrequency = 0;
//    byte hue = 0; будем использовать сдвиг от эффектов Радуга

byte waveRotation = 0;
uint8_t waveScale = 256 / WIDTH;
uint8_t waveCount = 1;

void WaveRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();

    //waveRotation = random(0, 4);// теперь вместо этого регулятор Масштаб
    waveRotation = (modes[currentMode].Scale - 1) / 25U;
    //waveCount = random(1, 3);// теперь вместо этого чётное/нечётное у регулятора Скорость
    waveCount = modes[currentMode].Speed % 2;
    //waveThetaUpdateFrequency = random(1, 2);
    //hueUpdateFrequency = random(1, 6);
  }

  dimAll(254);

  int n = 0;

  switch (waveRotation) {
    case 0:
      for (uint8_t x = 0; x < WIDTH; x++) {
        n = quadwave8(x * 2 + waveTheta) / waveScale;
        drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
        if (waveCount != 1)
          drawPixelXY(x, HEIGHT - 1 - n, ColorFromPalette(*curPalette, hue + x));
      }
      break;

    case 1:
      for (uint8_t y = 0; y < HEIGHT; y++) {
        n = quadwave8(y * 2 + waveTheta) / waveScale;
        drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
        if (waveCount != 1)
          drawPixelXY(WIDTH - 1 - n, y, ColorFromPalette(*curPalette, hue + y));
      }
      break;

    case 2:
      for (uint8_t x = 0; x < WIDTH; x++) {
        n = quadwave8(x * 2 - waveTheta) / waveScale;
        drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
        if (waveCount != 1)
          drawPixelXY(x, HEIGHT - 1 - n, ColorFromPalette(*curPalette, hue + x));
      }
      break;

    case 3:
      for (uint8_t y = 0; y < HEIGHT; y++) {
        n = quadwave8(y * 2 - waveTheta) / waveScale;
        drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
        if (waveCount != 1)
          drawPixelXY(WIDTH - 1 - n, y, ColorFromPalette(*curPalette, hue + y));
      }
      break;
  }


  if (waveThetaUpdate >= waveThetaUpdateFrequency) {
    waveThetaUpdate = 0;
    waveTheta++;
  }
  else {
    waveThetaUpdate++;
  }

  if (hueUpdate >= hueUpdateFrequency) {
    hueUpdate = 0;
    hue++;
  }
  else {
    hueUpdate++;
  }

  blurScreen(10); // @Palpalych советует делать размытие. вот в этом эффекте его явно не хватает...
}

//-------------------------Блуждающий кубик-----------------------
#define RANDOM_COLOR          (1U)                          // случайный цвет при отскоке
int16_t coordB[2U];
int8_t vectorB[2U];
CRGB ballColor;
//int8_t deltaValue; //ballSize;

void ballRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    //FastLED.clear();

    for (uint8_t i = 0U; i < 2U; i++)
    {
      coordB[i] = WIDTH / 2 * 10;
      vectorB[i] = random(8, 20);
    }
    deltaValue = map(modes[currentMode].Scale * 2.55, 0U, 255U, 2U, max((uint8_t)min(WIDTH, HEIGHT) / 3, 2));
    ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    //    _pulse_color = CHSV(random(0, 9) * 28, 255U, 255U);
  }

  //  if (!(modes[currentMode].Scale & 0x01))
  //  {
  //    hue += (modes[currentMode].Scale - 1U) % 11U * 8U + 1U;

  //    ballColor = CHSV(hue, 255U, 255U);
  //  }

  if ((modes[currentMode].Scale & 0x01))
    for (uint8_t i = 0U; i < deltaValue; i++)
      for (uint8_t j = 0U; j < deltaValue; j++)
        leds[XY(coordB[0U] / 10 + i, coordB[1U] / 10 + j)] = _pulse_color;

  for (uint8_t i = 0U; i < 2U; i++)
  {
    coordB[i] += vectorB[i];
    if (coordB[i] < 0)
    {
      coordB[i] = 0;
      vectorB[i] = -vectorB[i];
      if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U); // if (RANDOM_COLOR && (modes[currentMode].Scale & 0x01))
      //vectorB[i] += random(0, 6) - 3;
    }
  }
  if (coordB[0U] > (int16_t)((WIDTH - deltaValue) * 10))
  {
    coordB[0U] = (WIDTH - deltaValue) * 10;
    vectorB[0U] = -vectorB[0U];
    if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    //vectorB[0] += random(0, 6) - 3;
  }
  if (coordB[1U] > (int16_t)((HEIGHT - deltaValue) * 10))
  {
    coordB[1U] = (HEIGHT - deltaValue) * 10;
    vectorB[1U] = -vectorB[1U];
    if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    //vectorB[1] += random(0, 6) - 3;
  }

  //  if (modes[currentMode].Scale & 0x01)
    //  dimAll(135U);
     // dimAll(255U - (modes[currentMode].Scale - 1U) % 11U * 24U);
    //else
  FastLED.clear();

  for (uint8_t i = 0U; i < deltaValue; i++)
    for (uint8_t j = 0U; j < deltaValue; j++)
      leds[XY(coordB[0U] / 10 + i, coordB[1U] / 10 + j)] = ballColor;
}

//-------------------Светлячки со шлейфом----------------------------
#define BALLS_AMOUNT          (3U)                          // количество "шариков"
#define CLEAR_PATH            (1U)                          // очищать путь
#define TRACK_STEP            (130U)                         // длина хвоста шарика (чем больше цифра, тем хвост короче)
int16_t coord[BALLS_AMOUNT][2U];
int8_t vector[BALLS_AMOUNT][2U];
CRGB ballColors[BALLS_AMOUNT];
void ballsRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;

    for (uint8_t j = 0U; j < BALLS_AMOUNT; j++)
    {
      int8_t sign;
      // забиваем случайными данными
      coord[j][0U] = WIDTH / 2 * 10;
      random(0, 2) ? sign = 1 : sign = -1;
      vector[j][0U] = random(4, 15) * sign;
      coord[j][1U] = HEIGHT / 2 * 10;
      random(0, 2) ? sign = 1 : sign = -1;
      vector[j][1U] = random(4, 15) * sign;
      //ballColors[j] = CHSV(random(0, 9) * 28, 255U, 255U);
      // цвет зависит от масштаба
      ballColors[j] = CHSV((modes[currentMode].Scale * (j + 1)) % 256U, 255U, 255U);
    }
  }

  fader(TRACK_STEP);


  // движение шариков
  for (uint8_t j = 0U; j < BALLS_AMOUNT; j++)
  {
    // движение шариков
    for (uint8_t i = 0U; i < 2U; i++)
    {
      coord[j][i] += vector[j][i];
      if (coord[j][i] < 0)
      {
        coord[j][i] = 0;
        vector[j][i] = -vector[j][i];
      }
    }

    if (coord[j][0U] > (int16_t)((WIDTH - 1) * 10))
    {
      coord[j][0U] = (WIDTH - 1) * 10;
      vector[j][0U] = -vector[j][0U];
    }
    if (coord[j][1U] > (int16_t)((HEIGHT - 1) * 10))
    {
      coord[j][1U] = (HEIGHT - 1) * 10;
      vector[j][1U] = -vector[j][1U];
    }
    leds[XY(coord[j][0U] / 10, coord[j][1U] / 10)] =  ballColors[j];
  }
}

//-------------------Водопад------------------------------
#define COOLINGNEW 32
#define SPARKINGNEW 80
/*extern const TProgmemRGBPalette16 WaterfallColors4in1_p FL_PROGMEM = {CRGB::Black, CRGB::DarkSlateGray, CRGB::DimGray, CRGB::LightSlateGray, CRGB::DimGray, CRGB::DarkSlateGray, CRGB::Silver, CRGB::Lavender, CRGB::Silver, CRGB::Azure, CRGB::LightGrey, CRGB::GhostWhite, CRGB::Silver, CRGB::White, CRGB::RoyalBlue};
  void fire2012WithPalette4in1() {
  uint8_t rCOOLINGNEW = constrain((uint16_t)(modes[currentMode].Scale % 16) * 32 / HEIGHT + 16, 1, 255) ;
  // Array of temperature readings at each simulation cell
  //static byte heat[WIDTH][HEIGHT]; будет noise3d[0][WIDTH][HEIGHT]

  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (unsigned int i = 0; i < HEIGHT; i++) {
      //noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, ((rCOOLINGNEW * 10) / HEIGHT) + 2));
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, rCOOLINGNEW));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = HEIGHT - 1; k >= 2; k--) {
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW) {
      int y = random8(2);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (unsigned int j = 0; j < HEIGHT; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(noise3d[0][x][j], 240);
      if  (modes[currentMode].Scale < 16) {            // Lavafall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(LavaColors_p, colorindex);
      } else if (modes[currentMode].Scale < 32) {      // Firefall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(HeatColors_p, colorindex);
      } else if (modes[currentMode].Scale < 48) {      // Waterfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(WaterfallColors4in1_p, colorindex);
      } else if (modes[currentMode].Scale < 64) {      // Skyfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(CloudColors_p, colorindex);
      } else if (modes[currentMode].Scale < 80) {      // Forestfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(ForestColors_p, colorindex);
      } else if (modes[currentMode].Scale < 96) {      // Rainbowfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(RainbowColors_p, colorindex);
      } else {                      // Aurora
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(RainbowStripeColors_p, colorindex);
      }
    }
  }
  }*/
void fire2012WithPalette() {
  //    bool fire_water = modes[currentMode].Scale <= 50;
  //    uint8_t COOLINGNEW = fire_water ? modes[currentMode].Scale * 2  + 20 : (100 - modes[currentMode].Scale ) *  2 + 20 ;
  //    uint8_t COOLINGNEW = modes[currentMode].Scale * 2  + 20 ;
  // Array of temperature readings at each simulation cell
  //static byte heat[WIDTH][HEIGHT]; будет noise3d[0][WIDTH][HEIGHT]

  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (int i = 0; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, ((COOLINGNEW * 10) / HEIGHT) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = HEIGHT - 1; k >= 2; k--) {
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW) {
      int y = random8(2);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < HEIGHT; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(noise3d[0][x][j], 240);
      if (modes[currentMode].Scale == 100)
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(WaterfallColors_p, colorindex);
      else
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(CRGBPalette16( CRGB::Black, CHSV(modes[currentMode].Scale * 2.57, 255U, 255U) , CHSV(modes[currentMode].Scale * 2.57, 128U, 255U) , CRGB::White), colorindex);// 2.57 вместо 2.55, потому что 100 для белого цвета
      //leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(fire_water ? HeatColors_p : OceanColors_p, colorindex);
    }
  }
}
//-----------------------Недо огонь---------------------------------------
//stepko
#define FOR_i(from, to) for(int i = (from); i < (to); i++)
#define FOR_j(from, to) for(int j = (from); j < (to); j++)
unsigned int  counter;
int STEP = modes[currentMode].Scale; //нужно виставить номер эффекта с пометкой false или любое число если не хотите Белого огня
void noiseWave(bool isColored) {
if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }
  FastLED.clear();
  
  FOR_i(0, WIDTH) {
    byte thisVal = sin8(STEP*counter*i);
    byte thisMax = map(thisVal, 0, 255, 0, HEIGHT);
    if (isColored) {
        FOR_j(0, thisMax) {
          CRGB color = (CRGB)ColorFromPalette(*curPalette, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND);
          drawPixelXY(i, j, color);}}
    else {
      STEP = modes[currentMode].Scale;
      FOR_j(0, thisMax) {
        drawPixelXY(i, j, ColorFromPalette(WaterfallColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
      }
    }
  }
  counter += 1;
}

//----------------Gifка----------------------
byte frameNum;
  void animation1() {
  frameNum++;
  if (frameNum >= (framesArray)) frameNum = 0;
  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++)
      drawPixelXY(i, j, gammaCorrection(expandColor(pgm_read_word(&framesArray[frameNum][HEIGHT - j - 1][i]))));
  }



// -------------- эффект пульс ------------
// Stefan Petrick's PULSE Effect mod by PalPalych for GyverLamp

void drawCircle(int16_t x0, int16_t y0, uint16_t radius, const CRGB & color) {
  int a = radius, b = 0;
  int radiusError = 1 - a;

  if (radius == 0) {
    drawPixelXY(x0, y0, color);
    return;
  }

  while (a >= b)  {
    drawPixelXY(a + x0, b + y0, color);
    drawPixelXY(b + x0, a + y0, color);
    drawPixelXY(-a + x0, b + y0, color);
    drawPixelXY(-b + x0, a + y0, color);
    drawPixelXY(-a + x0, -b + y0, color);
    drawPixelXY(-b + x0, -a + y0, color);
    drawPixelXY(a + x0, -b + y0, color);
    drawPixelXY(b + x0, -a + y0, color);
    b++;
    if (radiusError < 0)
      radiusError += 2 * b + 1;
    else
    {
      a--;
      radiusError += 2 * (b - a + 1);
    }
  }
}

CRGBPalette16 palette;
uint8_t currentRadius = 4;
uint8_t pulse_centerX = random8(WIDTH - 5U) + 3U;
uint8_t pulse_centerY = random8(HEIGHT - 5U) + 3U;
//uint16_t _rc; вроде, не используется
//uint8_t _pulse_hue; заменено на deltaHue из общих переменных
//uint8_t _pulse_hueall; заменено на hue2 из общих переменных
//uint8_t _pulse_delta; заменено на deltaHue2 из общих переменных
//uint8_t pulse_hue; заменено на hue из общих переменных

void pulseRoutine(uint8_t PMode) {
  palette = RainbowColors_p;
  const uint8_t limitSteps = 6U;
  static const float fadeRate = 0.8;

  dimAll(248U);
  uint8_t _sat;
  if (step <= currentRadius) {
    for (uint8_t i = 0; i < step; i++ ) {
      uint8_t _dark = qmul8( 2U, cos8 (128U / (step + 1U) * (i + 1U))) ;
      switch (PMode) {
        case 1U:                    // 1 - случайные диски
          deltaHue = hue;
          _pulse_color = CHSV(deltaHue, 255U, _dark);
          break;
        case 2U:                    // 2...17 - перелив цвета дисков
          deltaHue2 = modes[currentMode].Scale;
          _pulse_color = CHSV(hue2, 255U, _dark);
          break;
        case 3U:                    // 18...33 - выбор цвета дисков
          deltaHue = modes[currentMode].Scale * 2.55;
          _pulse_color = CHSV(deltaHue, 255U, _dark);
          break;
        case 4U:                    // 34...50 - дискоцветы
          deltaHue += modes[currentMode].Scale;
          _pulse_color = CHSV(deltaHue, 255U, _dark);
          break;
        case 5U:                    // 51...67 - пузыри цветы
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue += modes[currentMode].Scale;
          _pulse_color = CHSV(deltaHue, _sat, _dark);
          break;
        case 6U:                    // 68...83 - выбор цвета пузырей
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue = modes[currentMode].Scale * 2.55;
          _pulse_color = CHSV(deltaHue, _sat, _dark);
          break;
        case 7U:                    // 84...99 - перелив цвета пузырей
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue2 = modes[currentMode].Scale;
          _pulse_color = CHSV(hue2, _sat, _dark);
          break;
        case 8U:                    // 100 - случайные пузыри
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue = hue;
          _pulse_color = CHSV(deltaHue, _sat, _dark);
          break;
      }
      drawCircle(pulse_centerX, pulse_centerY, i, _pulse_color  );
    }
  } else {
    pulse_centerX = random8(WIDTH - 5U) + 3U;
    pulse_centerY = random8(HEIGHT - 5U) + 3U;
    hue2 += deltaHue2;
    hue = random8(0U, 255U);
    currentRadius = random8(3U, 9U);
    step = 0;
  }
  step++;
}
// ============= ЭФФЕКТ CНЕГ/МАТРИЦА/ДОЖДЬ ===============
// от @Shaitan
void RainRoutine()
{
  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    // заполняем случайно верхнюю строку
    CRGB thisColor = getPixColorXY(x, HEIGHT - 1U);
    if ((uint32_t)thisColor == 0U)
    {
      if (random8(0, 50) == 0U)
      {
        if (modes[currentMode].Scale == 1) drawPixelXY(x, HEIGHT - 1U, CHSV(random(0, 9) * 28, 255U, 255U)); // Радужный дождь
        else if (modes[currentMode].Scale >= 100) drawPixelXY(x, HEIGHT - 1U, 0xE0FFFF - 0x101010 * random(0, 4)); // Снег
        else
          drawPixelXY(x, HEIGHT - 1U, CHSV(modes[currentMode].Scale * 2.4 + random(0, 16), 255, 255)); // Цветной дождь
      }
    }
    else
      leds[XY(x, HEIGHT - 1U)] -= CHSV(0, 0, random(96, 128));
  }
  // сдвигаем всё вниз
  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    for (uint8_t y = 0U; y < HEIGHT - 1U; y++)
    {
      drawPixelXY(x, y, getPixColorXY(x, y + 1U));
    }
  }
}
// --------------------------- эффект спирали ----------------------
/*
   Aurora: https://github.com/pixelmatix/aurora
   https://github.com/pixelmatix/aurora/blob/sm3.0-64x64/PatternSpiro.h
   Copyright (c) 2014 Jason Coon
   Неполная адаптация SottNick
*/
byte spirotheta1 = 0;
byte spirotheta2 = 0;
//    byte spirohueoffset = 0; // будем использовать переменную сдвига оттенка hue из эффектов Радуга


const uint8_t spiroradiusx = WIDTH / 4;
const uint8_t spiroradiusy = HEIGHT / 4;

const uint8_t spirocenterX = WIDTH / 2-1;
const uint8_t spirocenterY = HEIGHT / 2-1;

const uint8_t spirominx = spirocenterX - spiroradiusx;
const uint8_t spiromaxx = spirocenterX + spiroradiusx + 1;
const uint8_t spirominy = spirocenterY - spiroradiusy;
const uint8_t spiromaxy = spirocenterY + spiroradiusy + 1;

uint8_t spirocount = 1;
uint8_t spirooffset = 256 / spirocount;
boolean spiroincrement = false;

boolean spirohandledChange = false;

uint8_t mapsin8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
  uint8_t beatsin = sin8(theta);
  uint8_t rangewidth = highest - lowest;
  uint8_t Scaledbeat = scale8(beatsin, rangewidth);
  uint8_t result = lowest + Scaledbeat;
  return result;
}

uint8_t mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
  uint8_t beatcos = cos8(theta);
  uint8_t rangewidth = highest - lowest;
  uint8_t Scaledbeat = scale8(beatcos, rangewidth);
  uint8_t result = lowest + Scaledbeat;
  return result;
}

uint8_t beatcos8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0)
{
  uint8_t beat = beat8(beats_per_minute, timebase);
  uint8_t beatcos = cos8(beat + phase_offset);
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8(beatcos, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}

void spiroRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }

  blurScreen(5);
  dimAll(255U - modes[currentMode].Speed / 10);

  boolean change = false;

  for (uint8_t i = 0; i < spirocount; i++) {
    uint8_t x = mapsin8(spirotheta1 + i * spirooffset, spirominx, spiromaxx);
    uint8_t y = mapcos8(spirotheta1 + i * spirooffset, spirominy, spiromaxy);

    uint8_t x2 = mapsin8(spirotheta2 + i * spirooffset, x - spiroradiusx, x + spiroradiusx);
    uint8_t y2 = mapcos8(spirotheta2 + i * spirooffset, y - spiroradiusy, y + spiroradiusy);


    //CRGB color = ColorFromPalette( PartyColors_p, (hue + i * spirooffset), 128U); // вообще-то палитра должна постоянно меняться, но до адаптации этого руки уже не дошли
    //CRGB color = ColorFromPalette(*curPalette, hue + i * spirooffset, 128U); // вот так уже прикручена к бегунку Масштаба. за
    //leds[XY(x2, y2)] += color;
    if (x2 < WIDTH && y2 < HEIGHT) // добавил проверки. не знаю, почему эффект подвисает без них
      leds[XY(x2, y2)] += (CRGB)ColorFromPalette(*curPalette, hue + i * spirooffset);

    if ((x2 == spirocenterX && y2 == spirocenterY) ||
        (x2 == spirocenterX && y2 == spirocenterY)) change = true;
  }

  spirotheta2 += 2;

  EVERY_N_MILLIS(12) {
    spirotheta1 += 1;
  }

  EVERY_N_MILLIS(75) {
    if (change && !spirohandledChange) {
      spirohandledChange = true;

      if (spirocount >= WIDTH || spirocount == 1) spiroincrement = !spiroincrement;

      if (spiroincrement) {
        if (spirocount >= 10)
          spirocount *= 2;
        else
          spirocount += 1;
      }
      else {
        if (spirocount > 4)
          spirocount /= 2;
        else
          spirocount -= 1;
      }

      spirooffset = 256 / spirocount;
    }

    if (!change) spirohandledChange = false;
  }

  EVERY_N_MILLIS(33) {
    hue += 1;
  }
}


      void driftRoutine() {
         if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }
      uint8_t dim = beatsin8(2, 230, 250);
      dimAll(dim);

      for (int i = 2; i <= WIDTH / 2; i++)
      {
        CRGB color = (CRGB)ColorFromPalette(*curPalette,(i - 2) * (240 / (WIDTH / 2)));

        uint8_t x = beatcos8((spirocenterX + 1 - i) * 2, spirocenterX - i, spirocenterX + i);
        uint8_t y = beatsin8((spirocenterY + 1 - i) * 2, spirocenterY - i, spirocenterY + i);

        drawPixelXY(x, y, color);
      }
    }

void drift2Routine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }
  uint8_t dim = beatsin8(2, 170, 250);
  dimAll(dim);
  //FastLED.clear();

  for (uint8_t i = 0; i < WIDTH; i++)
  {
    CRGB color;

    uint8_t x = 0;
    uint8_t y = 0;

    if (i < spirocenterX) {
      x = beatcos8((i + 1) * 20, i, WIDTH - i);
      y = beatsin8((i + 1) * 20, i, HEIGHT - i);
      color = (CRGB)ColorFromPalette(*curPalette, i * 14);
    }
    else
    {
      x = beatsin8((WIDTH - i) * 20, WIDTH - i, i + 1);
      y = beatcos8((HEIGHT - i) * 20, HEIGHT - i, i + 1);
      color = (CRGB)ColorFromPalette(*curPalette, (7 - i) * 14);
    }

    drawPixelXY(x, y,color);
  }
}

/*void infinityRoutine() { //не очень
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }
  dimAll(255U - modes[currentMode].Speed / 10);
  //FastLED.clear();

  // the horizontal position of the head of the infinity sign
  // oscillates from 0 to the maximum horizontal and back
  int x = beatsin8(15, 1, WIDTH - 1);

  // the vertical position of the head oscillates
  int y = (HEIGHT - 1) - beatsin8(30, HEIGHT / 4, ((HEIGHT / 4) * 3) - 1);

  // the hue oscillates from 0 to 255, overflowing back to 0
  hue++;

  CRGB color = (CRGB)ColorFromPalette(*curPalette, hue);

  drawPixelXY(x, y, color);
  drawPixelXY(x - 1, y, color);
}
*/

byte theta = 0;
void radarRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }
  dimAll(254);

  for (int offset = 0; offset < spirocenterX; offset++) {
    byte hue = 255 - (offset * (256 / spirocenterX) + hue);
    CRGB color = ColorFromPalette(*curPalette, hue);
    uint8_t x = mapcos8(theta, offset, (WIDTH - 1) - offset);
    uint8_t y = mapsin8(theta, offset, (HEIGHT - 1) - offset);
    leds[XY(x, y)] = color;

    EVERY_N_MILLIS(25) {
      theta += 2;
      hue += 1;
    }
  }
}

//-----------------Эффект Вышиванка-------------
byte count = 0;
byte dir = 1;
byte flip = 0;
byte generation = 0;
void MunchRoutine() {
   if (loadingFlag)
  {
    loadingFlag = false;
      setCurrentPalette();
  }
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT; y++) {
      leds[XY(x, y)] = (x ^ y ^ flip) < count ? ColorFromPalette(*curPalette, ((x ^ y) << 4) + generation) : CRGB::Black;
    }
  }

  count += dir;

  if (count <= 0 || count >= WIDTH) {
    dir = -dir;
  }

  if (count <= 0) {
    if (flip == 0)
      flip = 7;
    else
      flip = 0;
  }

  generation++;
}


    int time = 0;

    void PlasmaRoutine() {
      if (loadingFlag)
  {
    loadingFlag = false;
      setCurrentPalette();
  }
        for (int x = 0; x < WIDTH; x++) {
            for (int y = 0; y < HEIGHT; y++) {
                int16_t v = 0;
                uint8_t wibble = sin8(time);
                v += sin16(x * wibble * 2 + time);
                v += cos16(y * (128 - wibble) * 2 + time);
                v += sin16(y * x * cos8(-time) / 2);
                CRGB color = ColorFromPalette(*curPalette,(v >> 8) + 127);
                drawPixelXY(x, y, color);
            }
        }

        time += 1;

    }
// ------------------------------ ЭФФЕКТ КОЛЬЦА / КОДОВЫЙ ЗАМОК ----------------------
// (c) SottNick
// из-за повторного использоваия переменных от других эффектов теперь в этом коде невозможно что-то понять.
// поэтому для понимания придётся сперва заменить названия переменных на человеческие. но всё равно это песец, конечно.
//uint8_t deltaHue2; // максимальне количество пикселей в кольце (толщина кольца) от 1 до HEIGHT / 2 + 1
//uint8_t deltaHue; // количество колец от 2 до HEIGHT
//uint8_t noise3d[1][1][HEIGHT]; // начальный оттенок каждого кольца (оттенка из палитры) 0-255
//uint8_t shiftValue[HEIGHT]; // местоположение начального оттенка кольца 0-WIDTH-1
//uint8_t shiftHue[HEIGHT]; // 4 бита на ringHueShift, 4 на ringHueShift2
////ringHueShift[ringsCount]; // шаг градиета оттенка внутри кольца -8 - +8 случайное число
////ringHueShift2[ringsCount]; // обычная скорость переливания оттенка всего кольца -8 - +8 случайное число
//uint8_t deltaValue; // кольцо, которое в настоящий момент нужно провернуть
//uint8_t step; // оставшееся количество шагов, на которое нужно провернуть активное кольцо - случайное от WIDTH/5 до WIDTH-3
//uint8_t hue, hue2; // количество пикселей в нижнем (hue) и верхнем (hue2) кольцах

void ringsRoutine() {
  uint8_t h, x, y;
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();

    //deltaHue2 = (modes[currentMode].Scale - 1U) / 99.0 * (HEIGHT / 2 - 1U) + 1U; // толщина кольца в пикселях. если на весь бегунок масштаба (от 1 до HEIGHT / 2 + 1)
    deltaHue2 = (modes[currentMode].Scale - 1U) % 11U + 1U; // толщина кольца от 1 до 11 для каждой из палитр
    deltaHue = HEIGHT / deltaHue2 + ((HEIGHT % deltaHue2 == 0U) ? 0U : 1U); // количество колец
    hue2 = deltaHue2 - (deltaHue2 * deltaHue - HEIGHT) / 2U; // толщина верхнего кольца. может быть меньше нижнего
    hue = HEIGHT - hue2 - (deltaHue - 2U) * deltaHue2; // толщина нижнего кольца = всё оставшееся
    for (uint8_t i = 0; i < deltaHue; i++)
    {
      noise3d[0][0][i] = random8(257U - WIDTH / 2U); // начальный оттенок кольца из палитры 0-255 за минусом длины кольца, делённой пополам
      shiftHue[i] = random8();
      shiftValue[i] = 0U; //random8(WIDTH); само прокрутится постепенно
      step = 0U;
      //do { // песец конструкцию придумал бредовую
      //  step = WIDTH - 3U - random8((WIDTH - 3U) * 2U); само присвоится при первом цикле
      //} while (step < WIDTH / 5U || step > 255U - WIDTH / 5U);
      deltaValue = random8(deltaHue);
    }

  }
  for (uint8_t i = 0; i < deltaHue; i++)
  {
    if (i != deltaValue) // если это не активное кольцо
    {
      h = shiftHue[i] & 0x0F; // сдвигаем оттенок внутри кольца
      if (h > 8U)
        //noise3d[0][0][i] += (uint8_t)(7U - h); // с такой скоростью сдвиг оттенка от вращения кольца не отличается
        noise3d[0][0][i]--;
      else
        //noise3d[0][0][i] += h;
        noise3d[0][0][i]++;
    }
    else
    {
      if (step == 0) // если сдвиг активного кольца завершён, выбираем следующее
      {
        deltaValue = random8(deltaHue);
        do {
          step = WIDTH - 3U - random8((WIDTH - 3U) * 2U); // проворот кольца от хз до хз
        } while (step < WIDTH / 5U || step > 255U - WIDTH / 5U);
      }
      else
      {
        if (step > 127U)
        {
          step++;
          shiftValue[i] = (shiftValue[i] + 1U) % WIDTH;
        }
        else
        {
          step--;
          shiftValue[i] = (shiftValue[i] - 1U + WIDTH) % WIDTH;
        }
      }
    }
    // отрисовываем кольца
    h = (shiftHue[i] >> 4) & 0x0F; // берём шаг для градиента вутри кольца
    if (h > 8U)
      h = 7U - h;
    for (uint8_t j = 0U; j < ((i == 0U) ? hue : ((i == deltaHue - 1U) ? hue2 : deltaHue2)); j++) // от 0 до (толщина кольца - 1)
    {
      y = i * deltaHue2 + j - ((i == 0U) ? 0U : deltaHue2 - hue);
      // mod для чётных скоростей by @kostyamat - получается какая-то другая фигня. не стоит того
      //for (uint8_t k = 0; k < WIDTH / ((modes[currentMode].Speed & 0x01) ? 2U : 4U); k++) // полукольцо для нечётных скоростей и четверть кольца для чётных
      for (uint8_t k = 0; k < WIDTH / 2U; k++) // полукольцо
      {
        x = (shiftValue[i] + k) % WIDTH; // первая половина кольца
        drawPixelXY(x, y, ColorFromPalette(*curPalette, noise3d[0][0][i] + k * h));
        x = (WIDTH - 1 + shiftValue[i] - k) % WIDTH; // вторая половина кольца (зеркальная первой)

        drawPixelXY(x, y, ColorFromPalette(*curPalette, noise3d[0][0][i] + k * h));
      }
      if (WIDTH & 0x01) //(WIDTH % 2U > 0U) // если число пикселей по ширине матрицы нечётное, тогда не забываем и про среднее значение
      {
        x = (shiftValue[i] + WIDTH / 2U) % WIDTH;
        drawPixelXY(x, y, ColorFromPalette(*curPalette, noise3d[0][0][i] + WIDTH / 2U * h));
      }
    }
  }
}


// ------------------------------ ЭФФЕКТ ЗВЁЗДЫ ----------------------
// (c) SottNick
// производная от эффекта White Warp
// https://github.com/marcmerlin/NeoMatrix-FastLED-IR/blob/master/Table_Mark_Estes_Impl.h
// https://github.com/marcmerlin/FastLED_NeoMatrix_SmartMatrix_LEDMatrix_GFX_Demos/blob/master/LEDMatrix/Table_Mark_Estes/Table_Mark_Estes.ino
//int16_t pointy, blender = 128;//, laps, hue, steper,  xblender, hhowmany, radius3, xpoffset[MATRIX_WIDTH * 3];
#define STAR_BLENDER 128U             // хз что это 
#define CENTER_DRIFT_SPEED 6U         // скорость перемещения плавающего центра возникновения звёзд
#define bballsMaxNUM            (WIDTH * 2)
//, ringdelay;//, bringdelay, sumthum;
//int16_t shifty = 6;//, pattern = 0, poffset;
int16_t radius2;//, fpeed[WIDTH * 3], fcount[WIDTH * 3], fcountr[WIDTH * 3];//, xxx, yyy, dot = 3, rr, gg, bb, adjunct = 3;
//uint8_t fcolor[WIDTH * 3];
uint16_t ccoolloorr; //, why1, why2, why3, eeks1, eeks2, eeks3, oldpattern, xhowmany, kk;
float driftx, drifty;//, locusx, locusy, xcen, ycen, yangle, xangle;
float cangle, sangle;//xfire[WIDTH * 3], yfire[WIDTH * 3], radius, xslope[MATRIX_WIDTH * 3], yslope[MATRIX_WIDTH * 3];

//Дополнительная функция построения линий
void DrawLine(int x1, int y1, int x2, int y2, CRGB color)
{
  int tmp;
  int x, y;
  int dx, dy;
  int err;
  int ystep;

  uint8_t swapxy = 0;

  if ( x1 > x2 ) dx = x1 - x2; else dx = x2 - x1;
  if ( y1 > y2 ) dy = y1 - y2; else dy = y2 - y1;

  if ( dy > dx )
  {
    swapxy = 1;
    tmp = dx; dx = dy; dy = tmp;
    tmp = x1; x1 = y1; y1 = tmp;
    tmp = x2; x2 = y2; y2 = tmp;
  }
  if ( x1 > x2 )
  {
    tmp = x1; x1 = x2; x2 = tmp;
    tmp = y1; y1 = y2; y2 = tmp;
  }
  err = dx >> 1;
  if ( y2 > y1 ) ystep = 1; else ystep = -1;
  y = y1;

  for ( x = x1; x <= x2; x++ )
  {
    if ( swapxy == 0 ) drawPixelXY(x, y, color);
    else drawPixelXY(y, x, color);
    err -= (uint8_t)dy;
    if ( err < 0 )
    {
      y += ystep;
      err += dx;
    }
  }
}

void drawstar(int16_t xlocl, int16_t ylocl, int16_t biggy, int16_t little, int16_t points, int16_t dangle, uint8_t koler)// random multipoint star
{
  //  if (counter == 0) { // это, блин, вообще что за хрень была?!
  //    shifty = 3;//move quick
  //  }
  radius2 = 255 / points;
  for (int i = 0; i < points; i++)
  {
    //DrawLine(xlocl + ((little * (sin8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128), ylocl + ((little * (cos8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128), xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128), ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128), CHSV(koler , 255, 255));
    //DrawLine(xlocl + ((little * (sin8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128), ylocl + ((little * (cos8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128), xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128), ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128), CHSV(koler , 255, 255));
    // две строчки выше - рисуют звезду просто по оттенку, а две строчки ниже - берут цвет из текущей палитры
    DrawLine(xlocl + ((little * (sin8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128), ylocl + ((little * (cos8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128), xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128), ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128), ColorFromPalette(*curPalette, koler));
    DrawLine(xlocl + ((little * (sin8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128), ylocl + ((little * (cos8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128), xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128), ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128), ColorFromPalette(*curPalette, koler));
  }
}

uint8_t bballsCOLOR[bballsMaxNUM] ;                   // цвет звезды (используем повторно массив эффекта Мячики)
uint8_t bballsX[bballsMaxNUM] ;                       // количество углов в звезде (используем повторно массив эффекта Мячики)
int   bballsPos[bballsMaxNUM] ;                       // задержка пуска звезды относительно счётчика (используем повторно массив эффекта Мячики)
uint8_t bballsNUM;                                    // количество звёзд (используем повторно переменную эффекта Мячики)

void starRoutine() {
  //dimAll(255U - modes[currentMode].Scale * 2);
  dimAll(89U);
  //dimAll(myscale8(modes[currentMode].Scale));

  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();

    driftx = random8(4, WIDTH - 4);//set an initial location for the animation center
    drifty = random8(4, HEIGHT - 4);// set an initial location for the animation center

    cangle = (sin8(random(25, 220)) - 128.0) / 128.0;//angle of movement for the center of animation gives a float value between -1 and 1
    sangle = (sin8(random(25, 220)) - 128.0) / 128.0;//angle of movement for the center of animation in the y direction gives a float value between -1 and 1
    //shifty = random (3, 12);//how often the drifter moves будет CENTER_DRIFT_SPEED = 6

    //pointy = 7; теперь количество углов у каждой звезды своё
    bballsNUM = (WIDTH + 6U) / 2U;//(modes[currentMode].Scale - 1U) / 99.0 * (bballsMaxNUM - 1U) + 1U;
    if (bballsNUM > bballsMaxNUM) bballsNUM = bballsMaxNUM;
    for (uint8_t num = 0; num < bballsNUM; num++) {
      bballsX[num] = random8(3, 9);//pointy = random8(3, 9); // количество углов в звезде
      bballsPos[num] = counter + (num << 2) + 1U;//random8(50);//modes[currentMode].Scale;//random8(50, 99); // задержка следующего пуска звезды
      bballsCOLOR[num] = random8();
    }

  }


  //hue++;//increment the color basis был общий оттенок на весь эффект. теперь у каждой звезды свой
  //h = hue;  //set h to the color basis
  counter++;
  if (driftx > (WIDTH - spirocenterX / 2U))//change directin of drift if you get near the right 1/4 of the screen
    cangle = 0 - fabs(cangle);
  if (driftx < spirocenterX / 2U)//change directin of drift if you get near the right 1/4 of the screen
    cangle = fabs(cangle);
  if (counter % CENTER_DRIFT_SPEED == 0)
    driftx = driftx + cangle;//move the x center every so often

  if (drifty > ( HEIGHT - spirocenterY / 2U))// if y gets too big, reverse
    sangle = 0 - fabs(sangle);
  if (drifty < spirocenterY / 2U) // if y gets too small reverse
    sangle = fabs(sangle);
  if ((counter + CENTER_DRIFT_SPEED / 2U) % CENTER_DRIFT_SPEED == 0)
    drifty =  drifty + sangle;//move the y center every so often

  //по идее, не нужно равнять диапазоны плавающего центра. за них и так вылет невозможен
  //driftx = constrain(driftx, spirocenterX - spirocenterX / 3, spirocenterX + spirocenterX / 3);//constrain the center, probably never gets evoked any more but was useful at one time to keep the graphics on the screen....
  //drifty = constrain(drifty, spirocenterY - spirocenterY / 3, spirocenterY + spirocenterY / 3);

  for (uint8_t num = 0; num < bballsNUM; num++) {
    if (counter >= bballsPos[num])//(counter >= ringdelay)
    {
      if (counter - bballsPos[num] <= WIDTH + 5U) {  //(counter - ringdelay <= WIDTH + 5){
        //drawstar(driftx  , drifty, 2 * (counter - ringdelay), (counter - ringdelay), pointy, blender + h, h * 2 + 85);
        drawstar(driftx  , drifty, 2 * (counter - bballsPos[num]), (counter - bballsPos[num]), bballsX[num], STAR_BLENDER + bballsCOLOR[num], bballsCOLOR[num] * 2);//, h * 2 + 85);// что, бл, за 85?!
        bballsCOLOR[num]++;
      }
      else
        //bballsX[num] = random8(3, 9);//pointy = random8(3, 9); // количество углов в звезде
        bballsPos[num] = counter + (bballsNUM << 1) + 1U;//random8(50, 99);//modes[currentMode].Scale;//random8(50, 99); // задержка следующего пуска звезды
    }
  }
}




// ============= ЭФФЕКТ ПРИЗМАТА ===============
// Prismata Loading Animation
// https://github.com/pixelmatix/aurora/blob/master/PatternPendulumWave.h
// Адаптация от (c) SottNick

void PrismataRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }

  EVERY_N_MILLIS(33) {
    hue++; // используем переменную сдвига оттенка из функций радуги, чтобы не занимать память
  }
  FastLED.clear();

  for (uint8_t x = 0; x < WIDTH; x++)
  {
    //uint8_t y = beatsin8(x + 1, 0, HEIGHT-1); // это я попытался распотрошить данную функцию до исходного кода и вставить в неё регулятор скорости
    // вместо 28 в оригинале было 280, умножения на .Speed не было, а вместо >>17 было (<<8)>>24. короче, оригинальная скорость достигается при бегунке .Speed=20
    uint8_t beat = (GET_MILLIS() * (accum88(x + 1)) * 28 * modes[currentMode].Speed) >> 17;
    uint8_t y = scale8(sin8(beat), HEIGHT - 1);
    //и получилось!!!

    drawPixelXY(x, y, ColorFromPalette(*curPalette, x * 7 + hue));
  }
}
// ------------- светлячки --------------
#define BALLS_AMOUNT2          (10U)                          // количество "cветлячков"
#define CLEAR_PATH2            (1U)                          // очищать путь       
int16_t coord2[BALLS_AMOUNT2][2U];
int8_t vector2[BALLS_AMOUNT2][2U];
CRGB ballColors2[BALLS_AMOUNT2];
void lightersRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;

    for (uint8_t j = 0U; j < BALLS_AMOUNT2; j++)
    {
      int8_t sign2;
      // забиваем случайными данными
      coord2[j][0U] = WIDTH / 2 * 10;
      random(0, 2) ? sign2 = 1 : sign2 = -1;
      vector2[j][0U] = random(4, 15) * sign2;
      coord2[j][1U] = HEIGHT / 2 * 10;
      random(0, 2) ? sign2 = 1 : sign2 = -1;
      vector2[j][1U] = random(4, 15) * sign2;
      //ballColors[j] = CHSV(random(0, 9) * 28, 255U, 255U);
      // цвет зависит от масштаба
      ballColors2[j] = CHSV((modes[currentMode].Scale * (j + 1)) % 256U, 255U, 255U);
    }
  }


  FastLED.clear();

  // движение шариков
  for (uint8_t j = 0U; j < BALLS_AMOUNT2; j++)
  {
    // движение шариков
    for (uint8_t i = 0U; i < 2U; i++)
    {
      coord2[j][i] += vector2[j][i];
      if (coord2[j][i] < 0)
      {
        coord2[j][i] = 0;
        vector2[j][i] = -vector2[j][i];
      }
    }

    if (coord2[j][0U] > (int16_t)((WIDTH - 1) * 10))
    {
      coord2[j][0U] = (WIDTH - 1) * 10;
      vector2[j][0U] = -vector2[j][0U];
    }
    if (coord2[j][1U] > (int16_t)((HEIGHT - 1) * 10))
    {
      coord2[j][1U] = (HEIGHT - 1) * 10;
      vector2[j][1U] = -vector2[j][1U];
    }
    leds[XY(coord2[j][0U] / 10, coord2[j][1U] / 10)] =  ballColors2[j];
  }
}

// --------------------------- эффект мячики ----------------------
//  BouncingBalls2014 is a program that lets you animate an LED strip
//  to look like a group of bouncing balls
//  Daniel Wilson, 2014
//  https://github.com/githubcdr/Arduino/blob/master/bouncingballs/bouncingballs.ino
//  With BIG thanks to the FastLED community!
//  адаптация от SottNick
#define bballsGRAVITY           (-9.81)              // Downward (negative) acceleration of gravity in m/s^2
#define bballsH0                (1)                  // Starting height, in meters, of the ball (strip length)
//#define bballsMaxNUM            (WIDTH * 2)          // максимальное количество мячиков прикручено при адаптации для бегунка Масштаб
//uint8_t bballsNUM;                                   // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way) ... количество мячиков теперь задаётся бегунком, а не константой
//uint8_t bballsCOLOR[bballsMaxNUM] ;                   // прикручено при адаптации для разноцветных мячиков
//uint8_t bballsX[bballsMaxNUM] ;                       // прикручено при адаптации для распределения мячиков по радиусу лампы
bool bballsShift[bballsMaxNUM] ;                      // прикручено при адаптации для того, чтобы мячики не стояли на месте
float bballsVImpact0 = sqrt( -2 * bballsGRAVITY * bballsH0 );  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
float bballsVImpact[bballsMaxNUM] ;                   // As time goes on the impact velocity will change, so make an array to store those values
//uint16_t   bballsPos[bballsMaxNUM] ;                       // The integer position of the dot on the strip (LED index)
long  bballsTLast[bballsMaxNUM] ;                     // The clock time of the last ground strike
float bballsCOR[bballsMaxNUM] ;                       // Coefficient of Restitution (bounce damping)

void BBallsRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    FastLED.clear();
    bballsNUM = (modes[currentMode].Scale - 1U) / 99.0 * (bballsMaxNUM - 1U) + 1U;
    if (bballsNUM > bballsMaxNUM) bballsNUM = bballsMaxNUM;
    for (uint8_t i = 0 ; i < bballsNUM ; i++) {             // Initialize variables
      bballsCOLOR[i] = random8();
      bballsX[i] = random8(0U, WIDTH);
      bballsTLast[i] = millis();
      bballsPos[i] = 0U;                                // Balls start on the ground
      bballsVImpact[i] = bballsVImpact0;                // And "pop" up at vImpact0
      bballsCOR[i] = 0.90 - float(i) / pow(bballsNUM, 2); // это, видимо, прыгучесть. для каждого мячика уникальная изначально
      bballsShift[i] = false;
      hue2 = (modes[currentMode].Speed > 127U) ? 255U : 0U;                                           // цветные или белые мячики
      hue = (modes[currentMode].Speed == 128U) ? 255U : 254U - modes[currentMode].Speed % 128U * 2U;  // скорость угасания хвостов 0 = моментально
    }
  }

  float bballsHi;
  float bballsTCycle;
  deltaHue++; // постепенное изменение оттенка мячиков (закомментировать строчку, если не нужно)
  dimAll(hue);
  for (uint8_t i = 0 ; i < bballsNUM ; i++) {
    //leds[XY(bballsX[i], bballsPos[i])] = CRGB::Black; // off for the next loop around  // теперь пиксели гасятся в dimAll()

    bballsTCycle =  millis() - bballsTLast[i] ; // Calculate the time since the last time the ball was on the ground

    // A little kinematics equation calculates positon as a function of time, acceleration (gravity) and intial velocity
    bballsHi = 0.5 * bballsGRAVITY * pow( bballsTCycle / 1000.0 , 2.0 ) + bballsVImpact[i] * bballsTCycle / 1000.0;

    if ( bballsHi < 0 ) {
      bballsTLast[i] = millis();
      bballsHi = 0; // If the ball crossed the threshold of the "ground," put it back on the ground
      bballsVImpact[i] = bballsCOR[i] * bballsVImpact[i] ; // and recalculate its new upward velocity as it's old velocity * COR

      if ( bballsVImpact[i] < 0.01 ) // If the ball is barely moving, "pop" it back up at vImpact0
      {
        bballsCOR[i] = 0.90 - float(random(0U, 9U)) / pow(random(4U, 9U), 2); // сделал, чтобы мячики меняли свою прыгучесть каждый цикл
        bballsShift[i] = bballsCOR[i] >= 0.89;                             // если мячик максимальной прыгучести, то разрешаем ему сдвинуться
        bballsVImpact[i] = bballsVImpact0;
      }
    }
    bballsPos[i] = round( bballsHi * (HEIGHT - 1) / bballsH0);             // Map "h" to a "pos" integer index position on the LED strip
    if (bballsShift[i] && (bballsPos[i] == HEIGHT - 1)) {                  // если мячик получил право, то пускай сдвинется на максимальной высоте 1 раз
      bballsShift[i] = false;
      if (bballsCOLOR[i] % 2 == 0) {                                       // чётные налево, нечётные направо
        if (bballsX[i] == 0U) bballsX[i] = WIDTH - 1U;
        else --bballsX[i];
      } else {
        if (bballsX[i] == WIDTH - 1U) bballsX[i] = 0U;
        else ++bballsX[i];
      }
    }
    leds[XY(bballsX[i], bballsPos[i])] = CHSV(bballsCOLOR[i] + deltaHue, hue2, 255U);
  }
}

/*// ------------------------------ ЭФФЕКТ ПРЫГУНЫ ----------------------
  // стырено откуда-то by @obliterator
  // https://github.com/DmytroKorniienko/FireLamp_JeeUI/blob/templ/src/effects.cpp

  //Leaper leapers[20];
  //вместо класса Leaper будем повторно использовать переменные из эффекта мячики
  //float x, y; будет:
  float leaperX[bballsMaxNUM];
  float leaperY[bballsMaxNUM];
  //float xd, yd; будет:
  ////float bballsVImpact[bballsMaxNUM];                   // As time goes on the impact velocity will change, so make an array to store those values
  ////float bballsCOR[bballsMaxNUM];                       // Coefficient of Restitution (bounce damping)
  //CHSV color; будет:
  ////uint8_t bballsCOLOR[bballsMaxNUM];

  void LeapersRestart_leaper(uint8_t l) {
  // leap up and to the side with some random component
  bballsVImpact[l] = (1 * (float)random8(1, 100) / 100);
  bballsCOR[l] = (2 * (float)random8(1, 100) / 100);

  // for variety, sometimes go 50% faster
  if (random8() < 12) {
    bballsVImpact[l] += bballsVImpact[l] * 0.5;
    bballsCOR[l] += bballsCOR[l] * 0.5;
  }

  // leap towards the centre of the screen
  if (leaperX[l] > (WIDTH / 2)) {
    bballsVImpact[l] = -bballsVImpact[l];
  }
  }

  void LeapersMove_leaper(uint8_t l) {
  #define GRAVITY            0.06
  #define SETTLED_THRESHOLD  0.1
  #define WALL_FRICTION      0.95
  #define WIND               0.95    // wind resistance

  leaperX[l] += bballsVImpact[l];
  leaperY[l] += bballsCOR[l];

  // bounce off the floor and ceiling?
  if (leaperY[l] < 0 || leaperY[l] > HEIGHT - 1) {
    bballsCOR[l] = (-bballsCOR[l] * WALL_FRICTION);
    bballsVImpact[l] = ( bballsVImpact[l] * WALL_FRICTION);
    leaperY[l] += bballsCOR[l];
    if (leaperY[l] < 0) leaperY[l] = 0;
    // settled on the floor?
    if (leaperY[l] <= SETTLED_THRESHOLD && fabs(bballsCOR[l]) <= SETTLED_THRESHOLD) {
      LeapersRestart_leaper(l);
    }
  }

  // bounce off the sides of the screen?
  if (leaperX[l] <= 0 || leaperX[l] >= WIDTH - 1) {
    bballsVImpact[l] = (-bballsVImpact[l] * WALL_FRICTION);
    if (leaperX[l] <= 0) {
      leaperX[l] = bballsVImpact[l];
    } else {
      leaperX[l] = WIDTH - 1 - bballsVImpact[l];
    }
  }

  bballsCOR[l] -= GRAVITY;
  bballsVImpact[l] *= WIND;
  bballsCOR[l] *= WIND;
  }


  void LeapersRoutine(){
  //unsigned num = map(Scale, 0U, 255U, 6U, sizeof(boids) / sizeof(*boids));
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
    //FastLED.clear();
    //bballsNUM = (modes[currentMode].Scale - 1U) / 99.0 * (bballsMaxNUM - 1U) + 1U;
    bballsNUM = (modes[currentMode].Scale - 1U) % 11U / 10.0 * (bballsMaxNUM - 1U) + 1U;
    if (bballsNUM > bballsMaxNUM) bballsNUM = bballsMaxNUM;
    //if (bballsNUM < 2U) bballsNUM = 2U;

    for (uint8_t i = 0 ; i < bballsNUM ; i++) {
      leaperX[i] = random8(WIDTH);
      leaperY[i] = random8(HEIGHT);

      //curr->color = CHSV(random(1U, 255U), 255U, 255U);
      bballsCOLOR[i] = random8();
    }
  }
  FastLED.clear();

  for (uint8_t i = 0; i < bballsNUM; i++) {
    LeapersMove_leaper(i);
    //drawPixelXYF(leaperX[i], leaperY[i], CHSV(bballsCOLOR[i], 255U, 255U));
    drawPixelXYF(leaperX[i], leaperY[i], ColorFromPalette(*curPalette, bballsCOLOR[i]));
  };

  blurScreen(20);
  }*/

  #define PAUSE_MAX 7 // пропустить 7 кадров после завершения анимации сдвига ячеек

//uint8_t noise3d[1][WIDTH][HEIGHT]; // тут используем только нулевую колонку и нулевую строку. просто для экономии памяти взяли существующий трёхмерный массив
//uint8_t hue2; // осталось шагов паузы
//uint8_t step; // текущий шаг сдвига (от 0 до deltaValue-1)
//uint8_t deltaValue; // всего шагов сдвига (до razmer? до (razmer?+1)*shtuk?)
//uint8_t deltaHue, deltaHue2; // глобальный X и глобальный Y нашего "кубика"
uint8_t razmerX, razmerY; // размеры ячеек по горизонтали / вертикали
uint8_t shtukX, shtukY; // количество ячеек по горизонтали / вертикали
uint8_t poleX, poleY; // размер всего поля по горизонтали / вертикали (в том числе 1 дополнительная пустая дорожка-разделитель с какой-то из сторон)
int8_t globalShiftX, globalShiftY; // нужно ли сдвинуть всё поле по окончаии цикла и в каком из направлений (-1, 0, +1)
bool seamlessX; // получилось ли сделать поле по Х бесшовным
bool krutimVertikalno; // направление вращения в данный момент

void cube2dRoutine(){
    uint8_t x, y;
    uint8_t anim0; // будем считать тут начальный пиксель для анимации сдвига строки/колонки
    int8_t shift, kudaVse; // какое-то расчётное направление сдвига (-1, 0, +1)
    CRGB color, color2;
    
    if (loadingFlag)
    {
      loadingFlag = false;
      setCurrentPalette();
      FastLED.clear();

      razmerX = (modes[currentMode].Scale - 1U) % 11U + 1U; // размер ячейки от 1 до 11 пикселей для каждой из 9 палитр
      razmerY = razmerX;
      if (modes[currentMode].Speed & 0x01) // по идее, ячейки не обязательно должны быть квадратными, поэтому можно тут поизвращаться
        razmerY = (razmerY << 1U) + 1U;

      shtukY = HEIGHT / (razmerY + 1U);
      if (shtukY < 2U)
        shtukY = 2U;
      y = HEIGHT / shtukY - 1U;
      if (razmerY > y)
        razmerY = y;
      poleY = (razmerY + 1U) * shtukY;
      shtukX = WIDTH / (razmerX + 1U);
      if (shtukX < 2U)
        shtukX = 2U;
      x = WIDTH / shtukX - 1U;
      if (razmerX > x)
        razmerX = x;
      poleX = (razmerX + 1U) * shtukX;
      seamlessX = (poleX == WIDTH);
      deltaHue = 0U;
      deltaHue2 = 0U;
      globalShiftX = 0;
      globalShiftY = 0;

      for (uint8_t j = 0U; j < shtukY; j++)
      {
        y = j * (razmerY + 1U); // + deltaHue2 т.к. оно =0U
        for (uint8_t i = 0U; i < shtukX; i++)
        {
          x = i * (razmerX + 1U); // + deltaHue т.к. оно =0U
          if (modes[currentMode].Scale == 100U)
            color = CHSV(45U, 0U, 128U + random8(128U));
          else  
            color = ColorFromPalette(*curPalette, random8());
          for (uint8_t k = 0U; k < razmerY; k++)
            for (uint8_t m = 0U; m < razmerX; m++)
              leds[XY(x+m, y+k)] = color;
        }
      }
      step = 4U; // текущий шаг сдвига первоначально с перебором (от 0 до deltaValue-1)
      deltaValue = 4U; // всего шагов сдвига (от razmer? до (razmer?+1) * shtuk?)
      hue2 = 0U; // осталось шагов паузы

      //это лишнее обнуление
      //krutimVertikalno = true;
      //for (uint8_t i = 0U; i < shtukX; i++)
      //  noise3d[0][i][0] = 0U;
    }

  //двигаем, что получилось...
  if (hue2 == 0 && step < deltaValue) // если пауза закончилась, а цикл вращения ещё не завершён
  {
    step++;
    if (krutimVertikalno)
    {
      for (uint8_t i = 0U; i < shtukX; i++)
      {
        x = (deltaHue + i * (razmerX + 1U)) % WIDTH;
        if (noise3d[0][i][0] > 0) // в нулевой ячейке храним оставшееся количество ходов прокрутки
        {
          noise3d[0][i][0]--;
          shift = noise3d[0][i][1] - 1; // в первой ячейке храним направление прокрутки

          if (globalShiftY == 0)
            anim0 = (deltaHue2 == 0U) ? 0U : deltaHue2 - 1U;
          else if (globalShiftY > 0)
            anim0 = deltaHue2;
          else
            anim0 = deltaHue2 - 1U;
          
          if (shift < 0) // если крутим столбец вниз
          {
            color = leds[XY(x, anim0)];                                   // берём цвет от нижней строчки
            for (uint8_t k = anim0; k < anim0+poleY-1; k++)
            {
              color2 = leds[XY(x,k+1)];                                   // берём цвет от строчки над нашей
              for (uint8_t m = x; m < x + razmerX; m++)
                leds[XY(m % WIDTH,k)] = color2;                           // копируем его на всю нашу строку
            }
            for   (uint8_t m = x; m < x + razmerX; m++)
              leds[XY(m % WIDTH,anim0+poleY-1)] = color;                  // цвет нижней строчки копируем на всю верхнюю
          }
          else if (shift > 0) // если крутим столбец вверх
          {
            color = leds[XY(x,anim0+poleY-1)];                            // берём цвет от верхней строчки
            for (uint8_t k = anim0+poleY-1; k > anim0 ; k--)
            {
              color2 = leds[XY(x,k-1)];                                   // берём цвет от строчки под нашей
              for (uint8_t m = x; m < x + razmerX; m++)
                leds[XY(m % WIDTH,k)] = color2;                           // копируем его на всю нашу строку
            }
            for   (uint8_t m = x; m < x + razmerX; m++)
              leds[XY(m % WIDTH, anim0)] = color;                         // цвет верхней строчки копируем на всю нижнюю
          }
        }
      }
    }
    else
    {
      for (uint8_t j = 0U; j < shtukY; j++)
      {
        y = deltaHue2 + j * (razmerY + 1U);
        if (noise3d[0][0][j] > 0) // в нулевой ячейке храним оставшееся количество ходов прокрутки
        {
          noise3d[0][0][j]--;
          shift = noise3d[0][1][j] - 1; // в первой ячейке храним направление прокрутки
      
          if (seamlessX)
            anim0 = 0U;
          else if (globalShiftX == 0)
            anim0 = (deltaHue == 0U) ? 0U : deltaHue - 1U;
          else if (globalShiftX > 0)
            anim0 = deltaHue;
          else
            anim0 = deltaHue - 1U;
          
          if (shift < 0) // если крутим строку влево
          {
            color = leds[XY(anim0, y)];                            // берём цвет от левой колонки (левого пикселя)
            for (uint8_t k = anim0; k < anim0+poleX-1; k++)
            {
              color2 = leds[XY(k+1, y)];                           // берём цвет от колонки (пикселя) правее
              for (uint8_t m = y; m < y + razmerY; m++)
                leds[XY(k, m)] = color2;                           // копируем его на всю нашу колонку
            }
            for   (uint8_t m = y; m < y + razmerY; m++)
              leds[XY(anim0+poleX-1, m)] = color;                  // цвет левой колонки копируем на всю правую
          }
          else if (shift > 0) // если крутим столбец вверх
          {
            color = leds[XY(anim0+poleX-1, y)];                    // берём цвет от правой колонки
            for (uint8_t k = anim0+poleX-1; k > anim0 ; k--)
            {
              color2 = leds[XY(k-1, y)];                           // берём цвет от колонки левее
              for (uint8_t m = y; m < y + razmerY; m++)
                leds[XY(k, m)] = color2;                           // копируем его на всю нашу колонку
            }
            for   (uint8_t m = y; m < y + razmerY; m++)
              leds[XY(anim0, m)] = color;                          // цвет правой колонки копируем на всю левую
          }
        }
      }
    }
   
  }
  else if (hue2 != 0U) // пропускаем кадры после прокрутки кубика (делаем паузу)
    hue2--;

  if (step >= deltaValue) // если цикл вращения завершён, меняем местами соответствующие ячейки (цвет в них) и точку первой ячейки
    {
      step = 0U; 
      hue2 = PAUSE_MAX;
      //если часть ячеек двигалась на 1 пиксель, пододвигаем глобальные координаты начала
      deltaHue2 = deltaHue2 + globalShiftY; //+= globalShiftY;
      globalShiftY = 0;
      //deltaHue += globalShiftX; для бесшовной не годится
      deltaHue = (WIDTH + deltaHue + globalShiftX) % WIDTH;
      globalShiftX = 0;

      //пришла пора выбрать следующие параметры вращения
      kudaVse = 0;
      krutimVertikalno = random8(2U);
      if (krutimVertikalno) // идём по горизонтали, крутим по вертикали (столбцы двигаются)
      {
        for (uint8_t i = 0U; i < shtukX; i++)
        {
          noise3d[0][i][1] = random8(3);
          shift = noise3d[0][i][1] - 1; // в первой ячейке храним направление прокрутки
          if (kudaVse == 0)
            kudaVse = shift;
          else if (shift != 0 && kudaVse != shift)
            kudaVse = 50;
        }
        deltaValue = razmerY + ((deltaHue2 - kudaVse >= 0 && deltaHue2 - kudaVse + poleY < (int)HEIGHT) ? random8(2U) : 1U);

/*        if (kudaVse == 0) // пытался сделать, чтобы при совпадении "весь кубик стоит" сдвинуть его весь на пиксель, но заколебался
        {
          deltaValue = razmerY;
          kudaVse = (random8(2)) ? 1 : -1;
          if (deltaHue2 - kudaVse < 0 || deltaHue2 - kudaVse + poleY >= (int)HEIGHT)
            kudaVse = 0 - kudaVse;
        }
*/
        if (deltaValue == razmerY) // значит полюбому kudaVse было = (-1, 0, +1) - и для нуля в том числе мы двигаем весь куб на 1 пиксель
        {
          globalShiftY = 1 - kudaVse; //временно на единичку больше, чем надо
          for (uint8_t i = 0U; i < shtukX; i++)
            if (noise3d[0][i][1] == 1U) // если ячейка никуда не планировала двигаться
            {
              noise3d[0][i][1] = globalShiftY;
              noise3d[0][i][0] = 1U; // в нулевой ячейке храним количество ходов сдвига
            }
            else
              noise3d[0][i][0] = deltaValue; // в нулевой ячейке храним количество ходов сдвига
          globalShiftY--;
        }
        else
        {
          x = 0;
          for (uint8_t i = 0U; i < shtukX; i++)
            if (noise3d[0][i][1] != 1U)
              {
                y = random8(shtukY);
                if (y > x)
                  x = y;
                noise3d[0][i][0] = deltaValue * (x + 1U); // в нулевой ячейке храним количество ходов сдвига
              }  
          deltaValue = deltaValue * (x + 1U);
        }      
              
      }
      else // идём по вертикали, крутим по горизонтали (строки двигаются)
      {
        for (uint8_t j = 0U; j < shtukY; j++)
        {
          noise3d[0][1][j] = random8(3);
          shift = noise3d[0][1][j] - 1; // в первой ячейке храним направление прокрутки
          if (kudaVse == 0)
            kudaVse = shift;
          else if (shift != 0 && kudaVse != shift)
            kudaVse = 50;
        }
        if (seamlessX)
          deltaValue = razmerX + ((kudaVse < 50) ? random8(2U) : 1U);
        else  
          deltaValue = razmerX + ((deltaHue - kudaVse >= 0 && deltaHue - kudaVse + poleX < (int)WIDTH) ? random8(2U) : 1U);
        
/*        if (kudaVse == 0) // пытался сделать, чтобы при совпадении "весь кубик стоит" сдвинуть его весь на пиксель, но заколебался
        {
          deltaValue = razmerX;
          kudaVse = (random8(2)) ? 1 : -1;
          if (deltaHue - kudaVse < 0 || deltaHue - kudaVse + poleX >= (int)WIDTH)
            kudaVse = 0 - kudaVse;
        }
*/          
        if (deltaValue == razmerX) // значит полюбому kudaVse было = (-1, 0, +1) - и для нуля в том числе мы двигаем весь куб на 1 пиксель
        {
          globalShiftX = 1 - kudaVse; //временно на единичку больше, чем надо
          for (uint8_t j = 0U; j < shtukY; j++)
            if (noise3d[0][1][j] == 1U) // если ячейка никуда не планировала двигаться
            {
              noise3d[0][1][j] = globalShiftX;
              noise3d[0][0][j] = 1U; // в нулевой ячейке храним количество ходов сдвига
            }
            else
              noise3d[0][0][j] = deltaValue; // в нулевой ячейке храним количество ходов сдвига
          globalShiftX--;
        }
        else
        {
          y = 0;
          for (uint8_t j = 0U; j < shtukY; j++)
            if (noise3d[0][1][j] != 1U)
              {
                x = random8(shtukX);
                if (x > y)
                  y = x;
                noise3d[0][0][j] = deltaValue * (x + 1U); // в нулевой ячейке храним количество ходов сдвига
              }  
          deltaValue = deltaValue * (y + 1U);
        }      
      }
   }
}
