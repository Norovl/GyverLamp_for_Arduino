uint32_t effTimer;
byte ind;

void effectsTick() {
  // if (!dawnFlag)
  {
    if (millis() - effTimer >= ((currentMode < 5 || currentMode > 13) ? modes[currentMode].speed : 50) ) {
      effTimer = millis();
      if(ONflag){
        switch (currentMode) {
          case 0: sparklesRoutine();
            break;
          case 1: fireRoutine();
            break;
          case 2: rainbowVertical();
            break;
          case 3: rainbowHorizontal();
            break;
          case 4: colorsRoutine();
            break;
          case 5: madnessNoise();
            break;
          case 6: cloudNoise();
            break;
          case 7: lavaNoise();
            break;
          case 8: plasmaNoise();
            break;
          case 9: rainbowNoise();
            break;
          case 10: rainbowStripeNoise();
            break;
          case 11: zebraNoise();
            break;
          case 12: forestNoise();
            break;
          case 13: oceanNoise();
            break;
          case 14: colorRoutine();
            break;
          case 15: snowRoutine();
            break;
          case 16: matrixRoutine();
            break;
          case 17: whiteLamp();
            //        case 17: lightersRoutine();
            break;
        }
        if(VERTGAUGE)
          GaugeShowVertical();
        else
          GaugeShowHorizontal();
      }
      FastLED.show();
    }
  }
}

void GaugeShowVertical() {
  switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
    case 1:
      if(currentMode==17)
        ind = sqrt((255.0/(BRIGHTNESS-MIN17BRIGHTNESS))*(modes[currentMode].brightness-MIN17BRIGHTNESS) + 1); // привести к полной шкале
      else
        ind = sqrt((255.0/BRIGHTNESS)*modes[currentMode].brightness + 1); // привести к полной шкале
      for (byte x = 0; x <= xCol*(xStep-1) ; x+=xStep) {
        for (byte y = 0; y < HEIGHT ; y++) {
          if (ind > y)
            drawPixelXY(x, y, CHSV(10, 255, 255));
          else
            drawPixelXY(x, y,  0);
        }
      }
      break;
    case 2:
      ind = sqrt(modes[currentMode].speed - 1);
      for (byte x = 0; x <= xCol*(xStep-1) ; x+=xStep) {
        for (byte y = 0; y <= HEIGHT ; y++) {
          if (ind <= y)
            drawPixelXY(x, HEIGHT-1-y, CHSV(100, 255, 255));
          else
            drawPixelXY(x, HEIGHT-1-y,  0);
        }
      }
      break;
    case 3:
      ind = sqrt(modes[currentMode].scale + 1);
      for (byte x = 0; x <= xCol*(xStep-1) ; x+=xStep) {
        for (byte y = 0; y < HEIGHT ; y++) {
          if (ind > y)
            drawPixelXY(x, y, CHSV(150, 255, 255));
          else
            drawPixelXY(x, y,  0);
        }
      }
      break;
  }
}

void GaugeShowHorizontal() {
  switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
    case 1:
      if(currentMode==17)
        ind = sqrt((255.0/(BRIGHTNESS-MIN17BRIGHTNESS))*(modes[currentMode].brightness-MIN17BRIGHTNESS) + 1); // привести к полной шкале
      else
        ind = sqrt((255.0/BRIGHTNESS)*modes[currentMode].brightness + 1); // привести к полной шкале
      for (byte y = 0; y <= yCol*(yStep-1) ; y+=yStep) {
        for (byte x = 0; x < WIDTH ; x++) {
          if (ind > x)
            drawPixelXY((x+y)%WIDTH, y, CHSV(10, 255, 255));
          else
            drawPixelXY((x+y)%WIDTH, y,  0);
        }
      }
      break;
    case 2:
      ind = sqrt(modes[currentMode].speed - 1);
      for (byte y = 0; y <= yCol*(yStep-1) ; y+=yStep) {
        for (byte x = 0; x <= WIDTH ; x++) {
          if (ind < x)
            drawPixelXY((WIDTH-x+y)%WIDTH, y, CHSV(100, 255, 255));
          else
            drawPixelXY((WIDTH-x+y)%WIDTH, y,  0);
        }
      }
      break;
    case 3:
      ind = sqrt(modes[currentMode].scale + 1);
      for (byte y = 0; y <= yCol*(yStep-1) ; y+=yStep) {
        for (byte x = 0; x < WIDTH ; x++) {
          if (ind > x)
            drawPixelXY((x+y)%WIDTH, y, CHSV(150, 255, 255));
          else
            drawPixelXY((x+y)%WIDTH, y,  0);
        }
      }
      break;
  }
}

void changePower() {    // плавное включение/выключение
  if (ONflag) {
    effectsTick();
    for (int i = 0; i < modes[currentMode].brightness; i += 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.setBrightness(modes[currentMode].brightness);
    delay(2);
    FastLED.show();
  } else {
    //effectsTick();
    for (int i = modes[currentMode].brightness; i > 8; i -= 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.clear();
    delay(2);
    FastLED.show();
  }
}
