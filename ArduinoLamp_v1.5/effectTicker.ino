uint32_t effTimer;
byte ind;
void effectsTick() {
  {
    if (ONflag && millis() - effTimer >= ((currentMode < 5 || currentMode > 13) ? modes[currentMode].Speed : 50) ) {
      effTimer = millis(); switch (currentMode) {
        //|номер   |название функции эффекта     |тоже надо|
        case 0 : sparklesRoutine();             break;
        case 1 : fireRoutine();                 break;
        case 2 : rainbowVertical();             break;
        case 3 : rainbowHorizontal();           break;
        case 4 : rainbowDiagonalRoutine();      break;
        case 5 : madnessNoise();                break;
        case 6 : cloudNoise();                  break;
        case 7 : lavaNoise();                   break;
        case 8 : plasmaNoise();                 break;
        case 9: rainbowNoise();                 break;
        case 10: rainbowStripeNoise();          break;
        case 11: zebraNoise();                  break;
        case 12: forestNoise();                 break;
        case 13: oceanNoise();                  break;
        case 14: heatNoise();                   break;
        case 15: smokeNoise();                  break;
        case 16: lavLampNoise();                break;
        case 17: colorRoutine();                break;
        case 18: colorsRoutine();               break;
        case 19: whiteLamp();                   break;
        case 20: RainRoutine();                 break;
        case 21: stormRoutine2(true);           break;
        case 22: stormRoutine2(false);          break;
        case 23: SinusoidRoutine();             break;
        case 24: MetaBallsRoutine();            break;
        case 25: ballRoutine();                 break;
        case 26: ballsRoutine();                break;
        case 27: BBallsRoutine();               break;
        case 28: fire2012WithPalette();         break;
        case 29: noiseWave(false);              break;
        case 30: noiseWave(true);               break;
        case 31: lightersRoutine();             break;
        case 32: pulseRoutine(1);               break;
        case 33: pulseRoutine(4);               break;
      }
      switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
        case 1:
          ind = sqrt(modes[currentMode].Brightness + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(0, y, CHSV(10, 255, 255));
            else drawPixelXY(0, y,  0);
          }
          break;
        case 2:
          ind = sqrt(modes[currentMode].Speed - 1);
          for (byte y = 0; y <= HEIGHT ; y++) {
            if (ind <= y) drawPixelXY(0, 15 - y, CHSV(100, 255, 255));
            else drawPixelXY(0, 15 - y,  0);
          }
          break;
        case 3:
          ind = sqrt(modes[currentMode].Scale + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(0, y, CHSV(150, 255, 255));
            else drawPixelXY(0, y,  0);
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
