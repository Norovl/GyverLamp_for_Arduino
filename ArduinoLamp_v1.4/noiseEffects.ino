// ************* НАСТРОЙКИ *************
// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

uint16_t speed = 20;                                        // speed is set dynamically once we've started up
uint16_t scale = 30;                                        // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
#define MAX_DIMENSION (max(WIDTH, HEIGHT))
#if (WIDTH > HEIGHT)
uint8_t noise[WIDTH][WIDTH];
#else
uint8_t noise[HEIGHT][HEIGHT];
#endif

CRGBPalette16 pPalette;

// This function sets up a palette of black and blue stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( pPalette, 16, CHSV(modes[currentMode].Scale * 2.5,255, 75));

  for(uint8_t i = 0; i < 6; i++) {
    pPalette[i] = CHSV(modes[currentMode].Scale * 2.5, 255, 255);
  }
}


CRGBPalette16 currentPalette(PartyColors_p);
uint8_t colorLoop = 1;
uint8_t ihue = 0;

void madnessNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
  }
  fillnoise8();
  for (uint8_t i = 0; i < WIDTH; i++)
  {
    for (uint8_t j = 0; j < HEIGHT; j++)
    {
      CRGB thisColor = CHSV(noise[j][i], 255, noise[i][j]);
      leds[XY(i, j)] = CHSV(noise[j][i], 255, noise[i][j]);
    }
  }
  ihue += 1;
}

void rainbowNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = RainbowColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
  }
  fillNoiseLED(0.125,0.0625,1);
}

void rainbowStripeNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = RainbowStripeColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
  }
  fillNoiseLED(0.125,0.0625,1);
}

void zebraNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
  }
  fillNoiseLED(0.125,0.0625,1);
}

void forestNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = ForestColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }
  fillNoiseLED(0.125,0.0625,1);
}

void oceanNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = OceanColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }

  fillNoiseLED(0.125,0.0625,1);
}

void plasmaNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = PartyColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 1;
  }
  fillNoiseLED(0.125,0.0625,1);
}

void cloudNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = CloudColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }
  fillNoiseLED(0.125,0.0625,1);
}

void lavaNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = LavaColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }
  fillNoiseLED(0.125,0.0625,1);
}

void heatNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = HeatColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }
  fillNoiseLED(1,0,0.125);
}

void smokeNoise()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    currentPalette = WaterfallColors_p;
    scale = modes[currentMode].Scale;
    speed = modes[currentMode].Speed;
    colorLoop = 0;
  }
  fillNoiseLED(1,0,0.125);
}
// ************* СЛУЖЕБНЫЕ *************
void fillNoiseLED(byte x_dir, byte y_dir, byte z_dir)
{
  uint8_t dataSmoothing = 0;
  if (speed < 50)
  {
    dataSmoothing = 200 - (speed * 4);
  }
  for (uint8_t i = 0; i < MAX_DIMENSION; i++)
  {
    int32_t ioffset = scale * i;
    for (uint8_t j = 0; j < MAX_DIMENSION; j++)
    {
      int32_t joffset = scale * j;

      uint8_t data = inoise8(x + ioffset, y + joffset, z);

      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if (dataSmoothing)
      {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[i][j] = data;
    }
  }
  z += speed*z_dir;
  x -= speed*x_dir;
  y += speed*y_dir;

  for (uint8_t i = 0; i < WIDTH; i++)
  {
    for (uint8_t j = 0; j < HEIGHT; j++)
    {
      uint8_t index = noise[j][i];
      uint8_t bri =   noise[i][j];
      // if this palette is a 'loop', add a slowly-changing base value
      if ( colorLoop)
      {
        index += ihue;
      }
      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if ( bri > 127 )
      {
        bri = 255;
      }
      else
      {
        bri = dim8_raw( bri * 2);
      }
      CRGB color = ColorFromPalette( currentPalette, index, bri);
      leds[XY(i, j)] = color;
    }
  }
  ihue += 1;
}

void fillnoise8()
{
  for (uint8_t i = 0; i < MAX_DIMENSION; i++)
  {
    int32_t ioffset = scale * i;
    for (uint8_t j = 0; j < MAX_DIMENSION; j++)
    {
      int32_t joffset = scale * j;
      noise[i][j] = inoise8(x + ioffset, y + joffset, z);
    }
  }
  z -= speed;
}
