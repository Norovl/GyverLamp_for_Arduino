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
CRGB ledsbuff[NUM_LEDS];
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
void DrawLine(int x1, int y1, int x2, int y2, CRGB color)
{
  int deltaX = abs(x2 - x1);
  int deltaY = abs(y2 - y1);
  int signX = x1 < x2 ? 1 : -1;
  int signY = y1 < y2 ? 1 : -1;
  int error = deltaX - deltaY;

  drawPixelXY(x2, y2, color);
  while (x1 != x2 || y1 != y2) {
      drawPixelXY(x1, y1, color);
      int error2 = error * 2;
      if (error2 > -deltaY) {
          error -= deltaY;
          x1 += signX;
      }
      if (error2 < deltaX) {
          error += deltaX;
          y1 += signY;
      }
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
#define SPARKLES 1        // вылетающие угольки вкл выкл
//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[8][16] PROGMEM = {
  {32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 },
  {64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 },
  {96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 },
  {128, 64 , 32 , 0  , 0  , 32 , 64 , 128, 128, 64 , 32 , 0  , 0  , 32 , 64 , 128},
  {160, 96 , 64 , 32 , 32 , 64 , 96 , 160, 160, 96 , 64 , 32 , 32 , 64 , 96 , 160},
  {192, 128, 96 , 64 , 64 , 96 , 128, 192, 192, 128, 96 , 64 , 64 , 96 , 128, 192},
  {255, 160, 128, 96 , 96 , 128, 160, 255, 255, 160, 128, 96 , 96 , 128, 160, 255},
  {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const unsigned char hueMask[8][16] PROGMEM = {
  {1 , 11, 19, 25, 25, 22, 11, 1 , 1 , 11, 19, 25, 25, 22, 11, 1 },
  {1 , 8 , 13, 19, 25, 19, 8 , 1 , 1 , 8 , 13, 19, 25, 19, 8 , 1 },
  {1 , 8 , 13, 16, 19, 16, 8 , 1 , 1 , 8 , 13, 16, 19, 16, 8 , 1 },
  {1 , 5 , 11, 13, 13, 13, 5 , 1 , 1 , 5 , 11, 13, 13, 13, 5 , 1 },
  {1 , 5 , 11, 11, 11, 11, 5 , 1 , 1 , 5 , 11, 11, 11, 11, 5 , 1 },
  {0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 , 0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 },
  {0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 , 0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 },
  {0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 }
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
byte hue;byte hue2;
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
void snowRoutine() {
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

// ------------------------------ МАТРИЦА ------------------------------
void matrixRoutine() {
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

// ------------------------------ БЕЛАЯ ЛАМПА ------------------------------
void whiteLamp() {
  for (byte y = 0; y < (HEIGHT / 2); y++) {
    CHSV color = CHSV(100, 1, constrain(modes[currentMode].Brightness - (long)modes[currentMode].Speed * modes[currentMode].Brightness / 255 * y / 2, 1, 255));
    for (byte x = 0; x < WIDTH; x++) {
      drawPixelXY(x, y + 8, color);
      drawPixelXY(x, 7 - y, color);
    }
  }
}
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
  float e_s3_speed = 0.004 * modes[currentMode].Speed + 0.015; // speed of the movement along the Lissajous curves
  float e_s3_size = 3 * (float)modes[currentMode].Scale / 100.0 + 2;  // amplitude of the curves

  float time_shift = float(millis() % (uint32_t)(30000 * (1.0 / ((float)modes[currentMode].Speed / 255))));

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      CRGB color;

      float cx = y + float(e_s3_size * (sinf (float(e_s3_speed * 0.003 * time_shift)))) - semiHeightMajor;  // the 8 centers the middle on a 16x16
      float cy = x + float(e_s3_size * (cosf (float(e_s3_speed * 0.0022 * time_shift)))) - semiWidthMajor;
      float v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.r = v;

      cx = x + float(e_s3_size * (sinf (e_s3_speed * float(0.0021 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_speed * float(0.002 * time_shift)))) - semiHeightMajor;
      v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.b = v;

      cx = x + float(e_s3_size * (sinf (e_s3_speed * float(0.0041 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_speed * float(0.0052 * time_shift)))) - semiHeightMajor;
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

  float speed = modes[currentMode].Speed / 127.0;

  // get some 2 random moving points
  uint8_t x2 = inoise8(millis() * speed, 25355, 685 ) / WIDTH;
  uint8_t y2 = inoise8(millis() * speed, 355, 11685 ) / HEIGHT;

  uint8_t x3 = inoise8(millis() * speed, 55355, 6685 ) / WIDTH;
  uint8_t y3 = inoise8(millis() * speed, 25355, 22685 ) / HEIGHT;

  // and one Lissajou function
  uint8_t x1 = beatsin8(23 * speed, 0, 15);
  uint8_t y1 = beatsin8(28 * speed, 0, 15);

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
  //    dimAll(135U);
  //    dimAll(255U - (modes[currentMode].Scale - 1U) % 11U * 24U);
  //  else
  FastLED.clear();

  for (uint8_t i = 0U; i < deltaValue; i++)
    for (uint8_t j = 0U; j < deltaValue; j++)
      leds[XY(coordB[0U] / 10 + i, coordB[1U] / 10 + j)] = ballColor;
}

//-------------------Светлячки со шлейфом----------------------------
#define BALLS_AMOUNT          (3U)                          // количество "шариков"
#define CLEAR_PATH            (1U)                          // очищать путь
#define BALL_TRACK            (1U)                          // (0 / 1) - вкл/выкл следы шариков
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

  if (!BALL_TRACK)                                          // режим без следов шариков
  {
    FastLED.clear();
  }
  else                                                      // режим со следами
  {
    fader(TRACK_STEP);
  }

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
  extern const TProgmemRGBPalette16 WaterfallColors4in1_p FL_PROGMEM = {CRGB::Black,CRGB::DarkSlateGray,CRGB::DimGray,CRGB::LightSlateGray,CRGB::DimGray,CRGB::DarkSlateGray,CRGB::Silver,CRGB::Lavender,CRGB::Silver,CRGB::Azure,CRGB::LightGrey,CRGB::GhostWhite,CRGB::Silver,CRGB::White,CRGB::RoyalBlue};
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
}
void fire2012WithPalette() {
  //    bool fire_water = modes[currentMode].Scale <= 50;
  //    uint8_t COOLINGNEW = fire_water ? modes[currentMode].scale * 2  + 20 : (100 - modes[currentMode].Scale ) *  2 + 20 ;
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
int counter = 0;
int STEP = modes[30].Scale; //нужно виставить номер эффекта с пометкой false или любое число если не хотите Белого огня 
void noiseWave(bool isColored) {
FastLED.clear();
FOR_i(0, WIDTH) {
byte thisVal = inoise8(i * STEP, counter);
byte thisMax = map(thisVal, 0, 255, 0, HEIGHT);
if (isColored) {
if  (modes[currentMode].Scale < 16) {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(LavaColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// LavaWave
} else if (modes[currentMode].Scale < 32) {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(HeatColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// FireWave 
} else if (modes[currentMode].Scale < 48) {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(OceanColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// OceanWave    
} else if (modes[currentMode].Scale < 64) {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(ForestColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// ForestWave
} else if (modes[currentMode].Scale < 80) {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(CloudColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// SkyWave
} else if (modes[currentMode].Scale < 96) {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(PartyColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// PartyWave
} else if (modes[currentMode].Scale < 112) {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(RainbowColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// RainbowWave
} else {
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(RainbowStripeColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}// RainbowStripeWave
}
} 
else {
STEP = modes[currentMode].Scale;
FOR_j(0, thisMax) {
drawPixelXY(i, j, ColorFromPalette(WaterfallColors_p, map(j, 0, thisMax, 250, 0), 255, LINEARBLEND));
}
}
}
counter += 30;
}

//----------------Gifка----------------------
/*byte frameNum;
void animation1() {
  frameNum++;
  if (frameNum >= 6) frameNum = 0;
  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++)
      drawPixelXY(i, j, gammaCorrection(expandColor(pgm_read_word(&framesArray[frameNum][HEIGHT - j - 1][i]))));
}
*/


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
      if (modes[currentMode].Scale==1) drawPixelXY(x, HEIGHT - 1U, CHSV(random(0, 9) * 28, 255U, 255U)); // Радужный дождь
      else
      if (modes[currentMode].Scale >= 100) drawPixelXY(x, HEIGHT - 1U, 0xE0FFFF - 0x101010 * random(0, 4)); // Снег
      else
      drawPixelXY(x, HEIGHT - 1U, CHSV(modes[currentMode].Scale*2.4+random(0, 16),255,255)); // Цветной дождь
      }
  }
    else
       leds[XY(x,HEIGHT - 1U)]-=CHSV(0,0,random(96, 128));
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

// ============= ЭФФЕКТЫ ОСАДКИ / ТУЧКА В БАНКЕ / ГРОЗА В БАНКЕ ===============
// https://github.com/marcmerlin/FastLED_NeoMatrix_SmartMatrix_LEDMatrix_GFX_Demos/blob/master/FastLED/Sublime_Demos/Sublime_Demos.ino
// там по ссылке ещё остались эффекты с 3 по 9 (в SimplePatternList перечислены)

//прикольная процедура добавляет блеск почти к любому эффекту после его отрисовки https://www.youtube.com/watch?v=aobtR1gIyIo
//void addGlitter( uint8_t chanceOfGlitter){
//  if ( random8() < chanceOfGlitter) leds[ random16(NUM_LEDS) ] += CRGB::White;
//}

//static uint8_t intensity = 42;  // будет бегунок масштаба

// Array of temp cells (used by fire, theMatrix, coloredRain, stormyRain)
// uint8_t **tempMatrix; = noise3d[0][WIDTH][HEIGHT]
// uint8_t *splashArray; = line[WIDTH] из эффекта Огонь

CRGB solidRainColor = CRGB(60,80,90);

void rain(byte backgroundDepth, byte maxBrightness, byte spawnFreq, byte tailLength, CRGB rainColor, bool splashes, bool clouds, bool storm)
{
  static uint16_t noiseX = random16();
  static uint16_t noiseY = random16();
  static uint16_t noiseZ = random16();

  CRGB lightningColor = CRGB(72,72,80);
  CRGBPalette16 rain_p( CRGB::Black, rainColor );
#ifdef SMARTMATRIX
  CRGBPalette16 rainClouds_p( CRGB::Black, CRGB(75,84,84), CRGB(49,75,75), CRGB::Black );
#else
  CRGBPalette16 rainClouds_p( CRGB::Black, CRGB(15,24,24), CRGB(9,15,15), CRGB::Black );
#endif

  fadeToBlackBy( leds, NUM_LEDS, 255-tailLength);

  // Loop for each column individually
  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Move each dot down one cell
    for (uint8_t i = 0; i < HEIGHT; i++) {
      if (noise3d[0][x][i] >= backgroundDepth) {  // Don't move empty cells
        if (i > 0) noise3d[0][x][wrapY(i-1)] = noise3d[0][x][i];
        noise3d[0][x][i] = 0;
      }
    }

    // Step 2.  Randomly spawn new dots at top
    if (random8() < spawnFreq) {
      noise3d[0][x][HEIGHT-1] = random(backgroundDepth, maxBrightness);
    }

    // Step 3. Map from tempMatrix cells to LED colors
    for (uint8_t y = 0; y < HEIGHT; y++) {
      if (noise3d[0][x][y] >= backgroundDepth) {  // Don't write out empty cells
        leds[XY(x,y)] = ColorFromPalette(rain_p, noise3d[0][x][y]);
      }
    }

    // Step 4. Add splash if called for
    if (splashes) {
      // FIXME, this is broken
      byte j = line[x];
      byte v = noise3d[0][x][0];

      if (j >= backgroundDepth) {
        leds[XY(wrapX(x-2),0)] = ColorFromPalette(rain_p, j/3);
        leds[XY(wrapX(x+2),0)] = ColorFromPalette(rain_p, j/3);
        line[x] = 0;   // Reset splash
      }

      if (v >= backgroundDepth) {
        leds[XY(wrapX(x-1),1)] = ColorFromPalette(rain_p, v/2);
        leds[XY(wrapX(x+1),1)] = ColorFromPalette(rain_p, v/2);
        line[x] = v; // Prep splash for next frame
      }
    }

    // Step 5. Add lightning if called for
    if (storm) {
      //uint8_t lightning[WIDTH][HEIGHT];
      // ESP32 does not like static arrays  https://github.com/espressif/arduino-esp32/issues/2567
      uint8_t *lightning = (uint8_t *) malloc(WIDTH * HEIGHT);
      while (lightning == NULL) { Serial.println("lightning malloc failed"); }


      if (random16() < 72) {    // Odds of a lightning bolt
        lightning[scale8(random8(), WIDTH-1) + (HEIGHT-1) * WIDTH] = 255;  // Random starting location
        for(uint8_t ly = HEIGHT-1; ly > 1; ly--) {
          for (uint8_t lx = 1; lx < WIDTH-1; lx++) {
            if (lightning[lx + ly * WIDTH] == 255) {
              lightning[lx + ly * WIDTH] = 0;
              uint8_t dir = random8(4);
              switch (dir) {
                case 0:
                  leds[XY(lx+1,ly-1)] = lightningColor;
                  lightning[(lx+1) + (ly-1) * WIDTH] = 255; // move down and right
                break;
                case 1:
                  leds[XY(lx,ly-1)] = CRGB(128,128,128); // я без понятия, почему у верхней молнии один оттенок, а у остальных - другой
                  lightning[lx + (ly-1) * WIDTH] = 255;    // move down
                break;
                case 2:
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx-1) + (ly-1) * WIDTH] = 255; // move down and left
                break;
                case 3:
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx-1) + (ly-1) * WIDTH] = 255; // fork down and left
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx+1) + (ly-1) * WIDTH] = 255; // fork down and right
                break;
              }
            }
          }
        }
      }
      free(lightning);
    }

    // Step 6. Add clouds if called for
    if (clouds) {
      uint16_t noiseScale = 250;  // A value of 1 will be so zoomed in, you'll mostly see solid colors. A value of 4011 will be very zoomed out and shimmery
      //const uint16_t cloudHeight = (HEIGHT*0.2)+1;
      const uint8_t cloudHeight = HEIGHT * 0.4 + 1; // это уже 40% c лишеним, но на высоких матрицах будет чуть меньше

      // This is the array that we keep our computed noise values in
      //static uint8_t noise[WIDTH][cloudHeight];
      static uint8_t *noise = (uint8_t *) malloc(WIDTH * cloudHeight);
      
      while (noise == NULL) { Serial.println("noise malloc failed"); }
      int xoffset = noiseScale * x + hue;

      for(uint8_t z = 0; z < cloudHeight; z++) {
        int yoffset = noiseScale * z - hue;
        uint8_t dataSmoothing = 192;
        uint8_t noiseData = qsub8(inoise8(noiseX + xoffset,noiseY + yoffset,noiseZ),16);
        noiseData = qadd8(noiseData,scale8(noiseData,39));
        noise[x * cloudHeight + z] = scale8( noise[x * cloudHeight + z], dataSmoothing) + scale8( noiseData, 256 - dataSmoothing);
        nblend(leds[XY(x,HEIGHT-z-1)], ColorFromPalette(rainClouds_p, noise[x * cloudHeight + z]), (cloudHeight-z)*(250/cloudHeight));
      }
      noiseZ ++;
    }
  }
}

uint8_t myScale8(uint8_t x) { // даёт масштабировать каждые 8 градаций (от 0 до 7) бегунка Масштаб в значения от 0 до 255 по типа синусоиде
  uint8_t x8 = x % 8U;
  uint8_t x4 = x8 % 4U;
  if (x4 == 0U)
    if (x8 == 0U)       return 0U;
    else                return 255U;
  else if (x8 < 4U)     return (1U   + x4 * 72U); // всего 7шт по 36U + 3U лишних = 255U (чтобы восхождение по синусоиде не было зеркально спуску)
//else
                        return (253U - x4 * 72U); // 253U = 255U - 2U
}

void coloredRain() // внимание! этот эффект заточен на работу бегунка Масштаб в диапазоне от 0 до 255. пока что единственный.
{
  // я хз, как прикрутить а 1 регулятор и длину хвостов и цвет капель
  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  //rain(60, 200, map8(intensity,5,100), 195, CRGB::Green, false, false, false); // было CRGB::Green
  if (modes[currentMode].Scale > 247U)
    rain(60, 200, map8(42,5,100), myScale8(modes[currentMode].Scale), solidRainColor, false, false, false);
  else
    rain(60, 200, map8(42,5,100), myScale8(modes[currentMode].Scale), CHSV(modes[currentMode].Scale, 255U, 255U), false, false, false);
}

void simpleRain()
{
  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  //rain(60, 200, map8(intensity,2,60), 10, solidRainColor, true, true, false);
  rain(60, 180,(modes[currentMode].Scale-1) * 2.58, 30, solidRainColor, true, true, false);
}

void stormyRain()
{
  // ( Depth of dots, maximum brightness, frequency of new dots, length of tails, color, splashes, clouds, ligthening )
  //rain(0, 90, map8(intensity,0,150)+60, 10, solidRainColor, true, true, true);
  rain(60, 160, (modes[currentMode].Scale-1) * 2.58, 30, solidRainColor, true, true, true);
}
// ============= ЭФФЕКТ ОГОНЬ 2018 ===============
// https://gist.github.com/StefanPetrick/819e873492f344ebebac5bcd2fdd8aa8
// https://gist.github.com/StefanPetrick/1ba4584e534ba99ca259c1103754e4c5
// Адаптация от (c) SottNick

// parameters and buffer for the noise array
// (вместо закомментированных строк используются массивы и переменные от эффекта Кометы для экономии памяти)
//define NUM_LAYERS 2 // менять бесполезно, так как в коде чётко использовано 2 слоя
uint32_t noise32_x[NUM_LAYERSMAX];
uint32_t noise32_y[NUM_LAYERSMAX];
uint32_t noise32_z[NUM_LAYERSMAX];
uint32_t scale32_x[NUM_LAYERSMAX];
uint32_t scale32_y[NUM_LAYERSMAX];
//uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT];

uint8_t fire18heat[NUM_LEDS];
// this finds the right index within a serpentine matrix

void Fire2018() {
  const uint8_t CentreY =  HEIGHT / 2 + (HEIGHT % 2);
  const uint8_t CentreX =  WIDTH / 2  + (WIDTH % 2) ;

  // some changing values
  uint16_t ctrl1 = inoise16(11 * millis(), 0, 0);
  uint16_t ctrl2 = inoise16(13 * millis(), 100000, 100000);
  uint16_t  ctrl = ((ctrl1 + ctrl2) / 2);

  // parameters for the heatmap
  uint16_t speed = 25;
  noise32_x[0] = 3 * ctrl * speed;
  noise32_y[0] = 20 * millis() * speed;
  noise32_z[0] = 5 * millis() * speed ;
  scale32_x[0] = ctrl1 / 2;
  scale32_y[0] = ctrl2 / 2;

  //calculate the noise data
  uint8_t layer = 0;

  for (uint8_t i = 0; i < WIDTH; i++) {
    uint32_t ioffset = scale32_x[layer] * (i - CentreX);
    for (uint8_t j = 0; j < HEIGHT; j++) {
      uint32_t joffset = scale32_y[layer] * (j - CentreY);
      uint16_t data = ((inoise16(noise32_x[layer] + ioffset, noise32_y[layer] + joffset, noise32_z[layer])) + 1);
      noise3d[layer][i][j] = data >> 8;
    }
  }

  // parameters for te brightness mask
  speed = 20;
  noise32_x[1] = 3 * ctrl * speed;
  noise32_y[1] = 20 * millis() * speed;
  noise32_z[1] = 5 * millis() * speed ;
  scale32_x[1] = ctrl1 / 2;
  scale32_y[1] = ctrl2 / 2;

  //calculate the noise data
  layer = 1;
  for (uint8_t i = 0; i < WIDTH; i++) {
    uint32_t ioffset = scale32_x[layer] * (i - CentreX);
    for (uint8_t j = 0; j < HEIGHT; j++) {
      uint32_t joffset = scale32_y[layer] * (j - CentreY);
      uint16_t data = ((inoise16(noise32_x[layer] + ioffset, noise32_y[layer] + joffset, noise32_z[layer])) + 1);
      noise3d[layer][i][j] = data >> 8;
    }
  }

  // draw lowest line - seed the fire
  for (uint8_t x = 0; x < WIDTH; x++) {
    fire18heat[XY(x, HEIGHT - 1)] =  noise3d[0][WIDTH - 1 - x][CentreY - 1]; // хз, почему взято с середины. вожможно, нужно просто с 7 строки вне зависимости от высоты матрицы
  }


  //copy everything one line up
  for (uint8_t y = 0; y < HEIGHT - 1; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      fire18heat[XY(x, y)] = fire18heat[XY(x, y + 1)];
    }
  }

  //dim
  for (uint8_t y = 0; y < HEIGHT - 1; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t dim = noise3d[0][x][y];
      // high value = high flames
      dim = dim / 1.7;
      dim = 255 - dim;
      fire18heat[XY(x, y)] = scale8(fire18heat[XY(x, y)] , dim);
    }
  }
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      // map the colors based on heatmap
      //leds[XY(x, HEIGHT - 1 - y)] = CRGB( fire18heat[XY(x, y)], 1 , 0);
      //leds[XY(x, HEIGHT - 1 - y)] = CRGB( fire18heat[XY(x, y)], fire18heat[XY(x, y)] * 0.153, 0);// * 0.153 - лучший оттенок
      leds[XY(x, HEIGHT - 1 - y)] = CRGB( fire18heat[XY(x, y)], (float)fire18heat[XY(x, y)] * modes[currentMode].Scale * 0.01, 0);
      

      //пытался понять, как регулировать оттенок пламени...
      //  if (modes[currentMode].Scale > 50)
      //    leds[XY(x, HEIGHT - 1 - y)] = CRGB( fire18heat[XY(x, y)], fire18heat[XY(x, y)] * (modes[currentMode].Scale % 50)  * 0.051, 0);
      //  else
      //    leds[XY(x, HEIGHT - 1 - y)] = CRGB( fire18heat[XY(x, y)], 1 , fire18heat[XY(x, y)] * modes[currentMode].Scale * 0.051);
      //примерно понял
   
      // dim the result based on 2nd noise layer
      leds[XY(x, HEIGHT - 1 - y)].nscale8(noise3d[1][x][y]);
    }
  }

}

// --------------------------- эффект спирали ----------------------
/*
 * Aurora: https://github.com/pixelmatix/aurora
 * https://github.com/pixelmatix/aurora/blob/sm3.0-64x64/PatternSpiro.h
 * Copyright (c) 2014 Jason Coon
 * Неполная адаптация SottNick
 */
    byte spirotheta1 = 0;
    byte spirotheta2 = 0;
//    byte spirohueoffset = 0; // будем использовать переменную сдвига оттенка hue из эффектов Радуга
    

    const uint8_t spiroradiusx = WIDTH / 4;
    const uint8_t spiroradiusy = HEIGHT / 4;
    
    const uint8_t spirocenterX = WIDTH / 2-0.5;
    const uint8_t spirocenterY = HEIGHT / 2-0.5;
    
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
  uint8_t scaledbeat = scale8(beatsin, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}

uint8_t mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
  uint8_t beatcos = cos8(theta);
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
if (x2<WIDTH && y2<HEIGHT) // добавил проверки. не знаю, почему эффект подвисает без них
        leds[XY(x2, y2)] += (CRGB)ColorFromPalette(*curPalette, hue + i * spirooffset);
        
        if((x2 == spirocenterX && y2 == spirocenterY) ||
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
            if(spirocount >= 10)
              spirocount *= 2;
            else
              spirocount += 1;
          }
          else {
            if(spirocount > 4)
              spirocount /= 2;
            else
              spirocount -= 1;
          }

          spirooffset = 256 / spirocount;
        }
        
        if(!change) spirohandledChange = false;
      }

      EVERY_N_MILLIS(33) {
        hue += 1;
      }
}

// ******************************** СИНУСОИДЫ *******************************
  #define WAVES_AMOUNT 2    // количество синусоид
 #define DEG_TO_RAD 0.017453
  int t;
  byte w[WAVES_AMOUNT];
  byte phi[WAVES_AMOUNT];
  byte A[WAVES_AMOUNT];
  CRGB waveColors[WAVES_AMOUNT];

  void wavesRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    for (byte j = 0; j < WAVES_AMOUNT; j++) {
      // забиваем случайными данными
      w[j] = random(17, 25);
      phi[j] = random(0, 360);
      A[j] = HEIGHT / 2 * random(4, 11) / 10;
      waveColors[j] = CHSV(random(0, 9) * 28, 255, 255);
    }
  }
 

    // сдвигаем все пиксели вправо
    for (int i = WIDTH - 1; i > 0; i--)
      for (int j = 0; j < HEIGHT; j++)
        drawPixelXY(i, j, getPixColorXY(i - 1, j));

    // увеличиваем "угол"
    t++;
    if (t > 360) t = 0;

    // заливаем чёрным левую линию
    for (byte i = 0; i < HEIGHT; i++) {
      drawPixelXY(0, i, 0x000000);
    }

    // генерируем позицию точки через синус
    for (byte j = 0; j < WAVES_AMOUNT; j++) {
      float value = HEIGHT / 2 + (float)A[j] * sin((float)w[j] * t * DEG_TO_RAD + (float)phi[j] * DEG_TO_RAD);
      leds[getPixelNumber(0, (byte)value)] = waveColors[j];
    }
  
  }
  
//далее будут эффекты заточены для лампы в.1 лиш нужно припаять ленты как матрицу(паралельная или зигзаг)

// ****************************** ОГОНЁК ****************************** разный тип матрицы - выглядить будет по разному
int16_t position;
boolean direction;
#define TRACK_STEP3 100

void lighter() {
  fader(TRACK_STEP3);
  if (direction) {
    position++;
    if (position > NUM_LEDS - 2) {
      direction = false;
    }
  } else {
    position--;
    if (position < 1) {
      direction = true;
    }
  }
  leds[position] =  CHSV(modes[currentMode].Scale * 2.5, 255, 255);
}
// ============= ЭФФЕКТ ОГОНЬ 2012 ===============
// там выше есть его копии для эффектов Водопад и Водопад 4 в 1
  // по идее, надо бы объединить и оптимизировать, но мелких отличий довольно много
  // based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino


  // дополнительные палитры для пламени
  // для записи в PROGMEM преобразовывал из 4 цветов в 16 на сайте https://colordesigner.io/gradient-generator, но не уверен, что это эквивалент CRGBPalette16()
  // значения цветовых констант тут: https://github.com/FastLED/FastLED/wiki/Pixel-reference
  extern const TProgmemRGBPalette16 WoodFireColors_p FL_PROGMEM = {CRGB::Black, 0x330e00, 0x661c00, 0x992900, 0xcc3700, CRGB::OrangeRed, 0xff5800, 0xff6b00, 0xff7f00, 0xff9200, CRGB::Orange, 0xffaf00, 0xffb900, 0xffc300, 0xffcd00, CRGB::Gold};             //* Orange
  extern const TProgmemRGBPalette16 NormalFire_p FL_PROGMEM = {CRGB::Black, 0x330000, 0x660000, 0x990000, 0xcc0000, CRGB::Red, 0xff0c00, 0xff1800, 0xff2400, 0xff3000, 0xff3c00, 0xff4800, 0xff5400, 0xff6000, 0xff6c00, 0xff7800};                             // пытаюсь сделать что-то более приличное
  extern const TProgmemRGBPalette16 NormalFire2_p FL_PROGMEM = {CRGB::Black, 0x560000, 0x6b0000, 0x820000, 0x9a0011, CRGB::FireBrick, 0xc22520, 0xd12a1c, 0xe12f17, 0xf0350f, 0xff3c00, 0xff6400, 0xff8300, 0xffa000, 0xffba00, 0xffd400};                      // пытаюсь сделать что-то более приличное
  extern const TProgmemRGBPalette16 LithiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x240707, 0x470e0e, 0x6b1414, 0x8e1b1b, CRGB::FireBrick, 0xc14244, 0xd16166, 0xe08187, 0xf0a0a9, CRGB::Pink, 0xff9ec0, 0xff7bb5, 0xff59a9, 0xff369e, CRGB::DeepPink};        //* Red
  extern const TProgmemRGBPalette16 SodiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x332100, 0x664200, 0x996300, 0xcc8400, CRGB::Orange, 0xffaf00, 0xffb900, 0xffc300, 0xffcd00, CRGB::Gold, 0xf8cd06, 0xf0c30d, 0xe9b913, 0xe1af1a, CRGB::Goldenrod};           //* Yellow
  extern const TProgmemRGBPalette16 CopperFireColors_p FL_PROGMEM = {CRGB::Black, 0x001a00, 0x003300, 0x004d00, 0x006600, CRGB::Green, 0x239909, 0x45b313, 0x68cc1c, 0x8ae626, CRGB::GreenYellow, 0x94f530, 0x7ceb30, 0x63e131, 0x4bd731, CRGB::LimeGreen};     //* Green
  extern const TProgmemRGBPalette16 AlcoholFireColors_p FL_PROGMEM = {CRGB::Black, 0x000033, 0x000066, 0x000099, 0x0000cc, CRGB::Blue, 0x0026ff, 0x004cff, 0x0073ff, 0x0099ff, CRGB::DeepSkyBlue, 0x1bc2fe, 0x36c5fd, 0x51c8fc, 0x6ccbfb, CRGB::LightSkyBlue};  //* Blue
  extern const TProgmemRGBPalette16 RubidiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x0f001a, 0x1e0034, 0x2d004e, 0x3c0068, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, 0x3c0084, 0x2d0086, 0x1e0087, 0x0f0089, CRGB::DarkBlue};        //* Indigo
  extern const TProgmemRGBPalette16 PotassiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x0f001a, 0x1e0034, 0x2d004e, 0x3c0068, CRGB::Indigo, 0x591694, 0x682da6, 0x7643b7, 0x855ac9, CRGB::MediumPurple, 0xa95ecd, 0xbe4bbe, 0xd439b0, 0xe926a1, CRGB::DeepPink}; //* Violet
  const TProgmemRGBPalette16 *firePalettes[] = {
    &WoodFireColors_p,
    &NormalFire_p,
    &NormalFire2_p,
    &LithiumFireColors_p,
    &SodiumFireColors_p,
    &CopperFireColors_p,
    &AlcoholFireColors_p,
    &RubidiumFireColors_p,
    &PotassiumFireColors_p};

  void fire2012again()
  {
  if (loadingFlag)
  {
    loadingFlag = false;
    if (modes[currentMode].Scale > 100) modes[currentMode].Scale = 100; // чтобы не было проблем при прошивке без очистки памяти
    if (modes[currentMode].Scale > 50)
      //fire_p = firePalettes[(int)((float)modes[currentMode].Scale/12)];
      //fire_p = firePalettes[(uint8_t)((modes[currentMode].Scale % 50)/5.56F)];
      curPalette = firePalettes[(uint8_t)((modes[currentMode].Scale - 50)/50.0F * ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
      curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale/50.0F * ((sizeof(palette_arr)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
  }

  #if HEIGHT/6 > 6
  #define FIRE_BASE 6
  #else
  #define FIRE_BASE HEIGHT/6+1
  #endif
  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  uint8_t cooling = 70;
  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  uint8_t sparking = 130;
  // SMOOTHING; How much blending should be done between frames
  // Lower = more blending and smoother flames. Higher = less blending and flickery flames
  const uint8_t fireSmoothing = 80;
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Loop for each column individually
  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random(0, ((cooling * 10) / HEIGHT) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = HEIGHT; k > 1; k--) {
      noise3d[0][x][wrapY(k)] = (noise3d[0][x][k - 1] + noise3d[0][x][wrapY(k - 2)] + noise3d[0][x][wrapY(k - 2)]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < sparking) {
      uint8_t j = random8(FIRE_BASE);
      noise3d[0][x][j] = qadd8(noise3d[0][x][j], random(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    // Blend new data with previous frame. Average data between neighbouring pixels
    for (uint8_t y = 0; y < HEIGHT; y++)
      nblend(leds[XY(x,y)], ColorFromPalette(*curPalette, ((noise3d[0][x][y]*0.7) + (noise3d[0][wrapX(x+1)][y]*0.3))), fireSmoothing);
  }
  }
// ------------- светлячки --------------
#define BALLS_AMOUNT2          (10U)                          // количество "cветлячков"
#define CLEAR_PATH2            (1U)                          // очищать путь
#define BALL_TRACK2            (0U)                          // (0 / 1) - вкл/выкл следы шариков
#define TRACK_STEP2            (70U)                         // длина хвоста шарика (чем больше цифра, тем хвост короче)
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

if (!BALL_TRACK2)                                          // режим без следов шариков
{
  FastLED.clear();
}
else                                                      // режим со следами
{
  fader(TRACK_STEP2);
}

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
#define bballsMaxNUM            (WIDTH * 2)          // максимальное количество мячиков прикручено при адаптации для бегунка Масштаб
uint8_t bballsNUM;                                   // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way) ... количество мячиков теперь задаётся бегунком, а не константой
uint8_t bballsCOLOR[bballsMaxNUM] ;                   // прикручено при адаптации для разноцветных мячиков
uint8_t bballsX[bballsMaxNUM] ;                       // прикручено при адаптации для распределения мячиков по радиусу лампы
bool bballsShift[bballsMaxNUM] ;                      // прикручено при адаптации для того, чтобы мячики не стояли на месте
float bballsVImpact0 = sqrt( -2 * bballsGRAVITY * bballsH0 );  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
float bballsVImpact[bballsMaxNUM] ;                   // As time goes on the impact velocity will change, so make an array to store those values
uint16_t   bballsPos[bballsMaxNUM] ;                       // The integer position of the dot on the strip (LED index)
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
      hue2 = (modes[currentMode].Scale > 127U) ? 255U : 0U;                                           // цветные или белые мячики
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
  //unsigned num = map(scale, 0U, 255U, 6U, sizeof(boids) / sizeof(*boids));
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

  //myLamp.dimAll(0); накой хрен делать затухание на 100%?
  FastLED.clear();

  for (uint8_t i = 0; i < bballsNUM; i++) {
    LeapersMove_leaper(i);
    //drawPixelXYF(leaperX[i], leaperY[i], CHSV(bballsCOLOR[i], 255U, 255U));
    drawPixelXYF(leaperX[i], leaperY[i], ColorFromPalette(*curPalette, bballsCOLOR[i]));
  };

  blurScreen(20);
}*/
