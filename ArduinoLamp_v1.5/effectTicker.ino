uint32_t effTimer;
byte ind;
void effectsTick() {
  {
    if (ONflag && millis() - effTimer >= ((currentMode < 6 || currentMode > 21) ? modes[currentMode].Speed : 50) ) {
      effTimer = millis(); switch (currentMode) {
        //|номер   |название функции эффекта   |тоже надо|
        case 0 : sparklesRoutine();             break;
        case 1 : fireRoutine();                 break;
        case 2 : rainbowVertical();             break;
        case 3 : rainbowHorizontal();           break;
        case 4 : rainbowDiagonalRoutine();      break;
        case 5 : WaveRoutine();                 break;
        case 6 : BBallsRoutine();               break;
        case 7 : SinusoidRoutine();             break;
        case 8 : MetaBallsRoutine();            break;
        case 9 : PrismataRoutine();             break;
        case 10: madnessNoise();                break;
        case 11: cloudNoise();                  break;
        case 12: lavaNoise();                   break;
        case 13: plasmaNoise();                 break;
        case 14: rainbowNoise();                break;
        case 15: rainbowStripeNoise();          break;
        case 16: zebraNoise();                  break;
        case 17: forestNoise();                 break;
        case 18: oceanNoise();                  break;
        case 19: heatNoise();                   break;
        case 20: smokeNoise();                  break;
        case 21: lavLampNoise();                break;
        case 22: colorRoutine();                break;
        case 23: colorsRoutine();               break;
        case 24: RainRoutine();                 break;
        case 25: stormRoutine2(true);           break;
        case 26: stormRoutine2(false);          break;
        case 27: ballRoutine();                 break;
        case 28: ballsRoutine();                break;
        case 29: fire2012WithPalette();         break;
        case 30: lightersRoutine();             break;
        case 31: pulseRoutine(1);               break;
        case 32: ringsRoutine();                break;
        case 33: starRoutine();                 break;
        case 34: MunchRoutine();                break;

      }
      switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
        case 1:
          ind = sqrt(modes[currentMode].Brightness + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(y, 0, CHSV(10, 255, 255));
            else drawPixelXY(y, 0,  0);
          }
          break;
        case 2:
          ind = sqrt(modes[currentMode].Speed + 1);
          for (byte y = 0; y <= HEIGHT ; y++) {
            if (ind <= y) drawPixelXY(y, 0, CHSV(100, 255, 255));
            else drawPixelXY(y, 0,  0);
          }
          break;
        case 3:
          ind = sqrt(modes[currentMode].Scale + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(y, 0, CHSV(150, 255, 255));
            else drawPixelXY(y, 0,  0);
          }
          break;

      }
      FastLED.show();
    }
  }
}

void changePower() {    // плавное включение/выключение
  if (ONflag) {
    effectsTick();
    for (int i = 0; i < modes[currentMode].Brightness; i += 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.setBrightness(modes[currentMode].Brightness);
    delay(2);
    FastLED.show();
  } else {
    effectsTick();
    for (int i = modes[currentMode].Brightness; i > 8; i -= 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.clear();
    delay(2);
    FastLED.show();
  }
}

void demo() {
  if (isDemo && ONflag && millis() >= DemTimer) {
    if (RANDOM_DEMO)
      currentMode = random8(MODE_AMOUNT); // если нужен следующий случайный эффект
    else
      currentMode = currentMode + 1U < MODE_AMOUNT ? currentMode + 1U : 0U; // если нужен следующий по списку эффект
    FastLED.clear();
    DemTimer = millis() + DEMOTIMELIMIT;
    loadingFlag = true;
  }
}
